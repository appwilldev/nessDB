/*
 * nessDB storage engine
 * Copyright (c) 2011-2012, BohuTANG <overred.shuttler at gmail dot com>
 * All rights reserved.
 * Code is licensed with BSD. See COPYING.BSD file.
 *
 */

#ifndef __USE_FILE_OFFSET64
	#define __USE_FILE_OFFSET64
#endif

#ifndef __USE_LARGEFILE64
	#define __USE_LARGEFILE64
#endif

#ifndef _LARGEFILE64_SOURCE
	#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "buffer.h"
#include "log.h"
#include "debug.h"

#define DB_MAGIC (2011)

int _file_exists(const char *path)
{
	int fd = open(path, O_RDWR);
	if (fd > -1) {
		close(fd);
		return 1;
	}
	return 0;
}

struct log *log_new(const char *basedir, int lsn, int islog)
{
	int result;
	struct log *l;
	char log_name[FILE_PATH_SIZE];
	char db_name[FILE_PATH_SIZE];

	l = malloc(sizeof(struct log));
	l->islog = islog;

	memset(log_name, 0, FILE_PATH_SIZE);
	snprintf(log_name, FILE_PATH_SIZE, "%s/%d.log", basedir, 0);
	memcpy(l->name, log_name, FILE_PATH_SIZE);

	memset(l->basedir, 0, FILE_PATH_SIZE);
	memcpy(l->basedir, basedir, FILE_PATH_SIZE);

	memset(db_name, 0, FILE_PATH_SIZE);
	snprintf(db_name, FILE_PATH_SIZE, "%s/ness.db", basedir);


	if (_file_exists(db_name)) {
		l->db_wfd = open(db_name, LSM_OPEN_FLAGS, 0644);
		l->db_alloc = lseek(l->db_wfd, 0, SEEK_END);
	} else {
		int magic = DB_MAGIC;

		l->db_wfd = open(db_name, LSM_CREAT_FLAGS, 0644);
		result = write(l->db_wfd, &magic, sizeof(int));
		if (result == -1) 
			perror("write magic error\n");
		l->db_alloc = sizeof(int);
	}

	l->buf = buffer_new(256);
	l->db_buf = buffer_new(1024);

	return l;
}

int _log_read(char *logname, struct skiplist *list)
{
	int rem, count = 0, del_count = 0;
	int fd = open(logname, O_RDWR, 0644);
	int size = lseek(fd, 0, SEEK_END);

	if (fd == -1)
		return 0;

	rem = size;
	if (size == 0)
		return 0;

	lseek(fd, 0, SEEK_SET);
	while(rem > 0) {
		int isize = 0;
		int klen;
		uint64_t off;
		char key[NESSDB_MAX_KEY_SIZE];
		char klenstr[4], offstr[8], optstr[4];

		memset(klenstr, 0, 4);
		memset(offstr, 0, 8);
		memset(optstr, 0, 4);
		
		/* read key length */
		read(fd, &klenstr, sizeof(int));
		klen = u32_from_big((unsigned char*)klenstr);
		isize += sizeof(int);
		
		/* read key */
		memset(key, 0, NESSDB_MAX_KEY_SIZE);
		read(fd, &key, klen);
		isize += klen;

		/* read data offset */
		read(fd, &offstr, sizeof(uint64_t));
		off = u64_from_big((unsigned char*)offstr);
		isize += sizeof(uint64_t);

		/* read opteration */
		read(fd, &optstr, 1);
		isize += 1;
		if (memcmp(optstr, "A", 1) == 0) {
			count++;
			skiplist_insert(list, key, off, ADD);
		} else {
			del_count++;
			skiplist_insert(list, key, off, DEL);
		}

		rem -= isize;
	}
	__DEBUG(LEVEL_DEBUG, "recovery count ADD#%d, DEL#%d", count, del_count);
	return 1;
}

int log_recovery(struct log *l, struct skiplist *list)
{
	DIR *dd;
	int ret = 0;
	int flag = 0;
	char new_log[FILE_PATH_SIZE];
	char old_log[FILE_PATH_SIZE];
	struct dirent *de;

	if (!l->islog)
		return 0;

	memset(new_log, 0, FILE_PATH_SIZE);
	memset(old_log, 0, FILE_PATH_SIZE);

	dd = opendir(l->basedir);
	while ((de = readdir(dd))) {
		char *p = strstr(de->d_name, ".log");
		if (p) {
			if (flag == 0) {
				memcpy(new_log, de->d_name, FILE_PATH_SIZE);
				flag |= 0x01;
			} else {
				memcpy(old_log, de->d_name, FILE_PATH_SIZE);
				flag |= 0x10;
			}
		}
	}
	closedir(dd);

	/* 
	 * Get the two log files:new and old 
	 * Read must be sequential,read old then read new
	 */
	if ((flag & 0x01) == 0x01) {
		memset(l->log_new, 0, FILE_PATH_SIZE);
		snprintf(l->log_new, FILE_PATH_SIZE, "%s/%s", l->basedir, new_log);
		__DEBUG(LEVEL_DEBUG, "prepare to recovery from new log#%s", l->log_new);
		ret = _log_read(l->log_new, list);
		if (ret == 0)
			return ret;
	}

	if ((flag & 0x10) == 0x10) {
		memset(l->log_old, 0, FILE_PATH_SIZE);
		snprintf(l->log_old, FILE_PATH_SIZE, "%s/%s", l->basedir, old_log);
		__DEBUG(LEVEL_DEBUG, "prepare to recovery from old log#%s", l->log_old);
		ret = _log_read(l->log_old, list);
		if (ret == 0)
			return ret;
	}
	return ret;
}

uint64_t log_append(struct log *l, struct slice *sk, struct slice *sv)
{
	int len;
	int db_len;
	char *line;
	char *db_line;
	struct buffer *buf = l->buf;
	struct buffer *db_buf = l->db_buf;
	uint64_t db_offset = l->db_alloc;

	/* DB write */
	if (sv) {
		buffer_putint(db_buf, sv->len);
		buffer_putnstr(db_buf, sv->data, sv->len);
		db_len = db_buf->NUL;
		db_line = buffer_detach(db_buf);

		if (write(l->db_wfd, db_line, db_len) != db_len) {
			__DEBUG(LEVEL_ERROR, "%s:length:<%d>", "ERROR: Data AOF **ERROR**", db_len);
			return db_offset;
		}
		l->db_alloc += db_len;
	}

	/* LOG write */
	if (l->islog) {
		buffer_putint(buf, sk->len);
		buffer_putnstr(buf, sk->data, sk->len);
		buffer_putlong(buf, db_offset);
		if(sv)
			buffer_putnstr(buf, "A", 1);
		else
			buffer_putnstr(buf, "D", 1);

		len = buf->NUL;
		line = buffer_detach(buf);

		if (write(l->idx_wfd, line, len) != len)
			__DEBUG(LEVEL_ERROR, "%s,buffer is:%s,buffer length:<%d>", "ERROR: Log AOF **ERROR**", line, len);
	}


	return db_offset;
}

void log_remove(struct log *l, int lsn)
{
	char log_name[FILE_PATH_SIZE];
	memset(log_name, 0 ,FILE_PATH_SIZE);
	snprintf(log_name, FILE_PATH_SIZE, "%s/%d.log", l->basedir, lsn);
	remove(log_name);
}

void log_next(struct log *l, int lsn)
{
	char log_name[FILE_PATH_SIZE];
	memset(log_name, 0 ,FILE_PATH_SIZE);
	snprintf(log_name, FILE_PATH_SIZE, "%s/%d.log", l->basedir, lsn);
	memcpy(l->name, log_name, FILE_PATH_SIZE);

	buffer_clear(l->buf);
	buffer_clear(l->db_buf);

	close(l->idx_wfd);
	l->idx_wfd = open(l->name, LSM_CREAT_FLAGS, 0644);
}

void log_free(struct log *l)
{
	if (l) {
		buffer_free(l->buf);
		close(l->idx_wfd);
		free(l);
	}
}
