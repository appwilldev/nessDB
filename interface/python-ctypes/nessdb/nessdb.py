#!/usr/bin/env python
# -*- coding: utf-8 -*-
# author : KDr2
#

import os
import sys
import ctypes
import nessdb_data_structure as nessds

#load nessdb library
_nessdb_library=ctypes.cdll.LoadLibrary(os.path.join(sys.prefix,"lib/dylib/libnessdb.so"))

#load libc
if sys.platform.startswith("linux"):
    _libc_library=ctypes.cdll.LoadLibrary("libc.so.6")
elif sys.platform.startswith("darwin"):
    _libc_library=ctypes.cdll.LoadLibrary("/usr/lib/libSystem.B.dylib")
else:
    raise Exception("don't kown how to load libc dynamic library")


class NessDB(object):

    def __init__(self,path,bufferpool=1024*1024*512,tolog=0):
        self.db_open=_nessdb_library.db_open
        self.db_open.restype = ctypes.c_void_p
        self.db=self.db_open(bufferpool,path,tolog)
        self.db=ctypes.cast(self.db,ctypes.c_void_p)
        
    def db_add(self,key,value):
        return _nessdb_library.db_add(self.db,
                                      ctypes.byref(nessds.new_slice(key)),
                                      ctypes.byref(nessds.new_slice(value)))
    
    def db_get(self,key):
        ret_slice=nessds.new_slice("")
        retv= _nessdb_library.db_get(self.db,
                                     ctypes.byref(nessds.new_slice(key)),
                                     ctypes.byref(ret_slice))
        if retv == 1:
            d=ctypes.cast(ret_slice.data,ctypes.c_char_p)
            ret = str(d.value)
            _libc_library.free(ret_slice.data)
            return ret
        else:
            return None

    def db_remove(self,key):
        return _nessdb_library.db_remove(self.db,
                                         ctypes.byref(nessds.new_slice("key")))

    def db_close(self):
        return _nessdb_library.close(self.db)

    
