#!/usr/bin/env python
# -*- coding: utf-8 -*-
# author : KDr2
#

import ctypes
import nessdb_data_structure as nessds

_nessdb_library=ctypes.cdll.LoadLibrary("libnessdb.so")

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
            #TODO free d!!!
            ret = str(d.value)
            return ret
        else:
            return None

        
        
    def db_remove(self,key):
        return _nessdb_library.db_remove(self.db,
                                         ctypes.byref(nessds.new_slice("key")))

    def db_close(self):
        return _nessdb_library.close(self.db)




    
