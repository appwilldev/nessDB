#!/usr/bin/env python
# -*- coding: utf-8 -*-
# author : KDr2
#


import nessdb
db=nessdb.NessDB('test.db')

db.db_add("key","231")
print db.db_get("key")

db.db_remove("key")
print db.db_get("key")

db.db_add("key","哈哈")
print db.db_get("key")


