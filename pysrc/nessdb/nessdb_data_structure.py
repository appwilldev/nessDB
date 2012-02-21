#!/usr/bin/env python
# -*- coding: utf-8 -*-
# author : KDr2
#


import ctypes

#
# struct slice{
#	char *data;
#	int len;
# }
#

class SLICE(ctypes.Structure):
    _fields_ = [("data", ctypes.c_void_p),
                ("len", ctypes.c_int)]


def new_slice(value):
    v=ctypes.create_string_buffer(value)
    l=ctypes.c_int(len(value))
    return SLICE(ctypes.cast(v,ctypes.c_void_p),len(value))

#
# struct nessdb {
#	struct llru *lru;
#	struct index *idx;
#	struct buffer *buf;
#	time_t start_time;
# }
#

class NESSDB(ctypes.Structure):
    _fields_=[("llru",ctypes.c_void_p),
              ("idx",ctypes.c_void_p),
              ("buf",ctypes.c_void_p),
              ("start_time",ctypes.c_ulong)]

    


    
