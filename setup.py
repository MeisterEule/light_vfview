#!/usr/bin/env python
from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext

import os

class vfd_build_ext(build_ext):
   def build_extensions(self):
      build_ext.build_extensions(self)


vfd_ext = Extension('vfd_parse',
                      sources = ['vfd_parse.cpp'],
                      extra_compile_args=[],
                      extra_objects=[])

                       

setup(name = 'vfd_parse',
      cmdclass={'build_ext': vfd_build_ext,},
      extra_link_args=["-fPIC"],
      version='0.10.0',
      description='Read vfd samples',
      ext_modules=[vfd_ext]) 
