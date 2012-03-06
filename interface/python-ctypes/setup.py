#!/usr/bin/env python
# -*- coding: utf-8 -*-
# author : KDr2
#


import os
import sys

from distutils.core import setup

here_path=os.path.abspath(os.path.dirname(__file__))

def pypy():
    import sys
    if not '__pypy__' in sys.builtin_module_names:
        print "Warnning: Your are installing py-NessDB implemented with ctypes!"

def main():
    parent_dir=os.path.dirname(os.path.dirname(here_path))
    cmd="cd %s;make clean;make so" % parent_dir
    os.system(cmd)
    setup(name='nessdb',
          version='develop',
          author='KDr2',
          author_email="killy.draw@gmail.com",
          url='https://github.com/appwilldev/nessDB/',
          py_modules=['nessdb',
                      'nessdb.nessdb',
                      'nessdb.nessdb_data_structure'
                      ],
          data_files=[('lib/dylib', [os.path.join(parent_dir,'libnessdb.so')])]
          )

if __name__=='__main__':
    pypy()
    main()
    
