#!/usr/bin/env python

"""
setup.py file for SWIG example
"""

from distutils.core import setup, Extension


pyipriv_module = Extension('_pyipriv',
                           sources=['pyipriv_wrap.cxx', 'pyipriv.cpp'],
                           runtime_library_dirs=["/home/conductor/engineering/ThirdParty/libipriv"],
                           library_dirs=["/home/conductor/engineering/iprivpg/src"],
                           libraries=['ipriv'],
                          )

setup (name = 'pyipriv',
       version = '0.1',
       author      = "SWIG Docs",
       description = """Simple swig example from docs""",
       ext_modules = [pyipriv_module],
       py_modules = ["pyipriv"],
       )
