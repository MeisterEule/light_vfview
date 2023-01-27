#!/usr/bin/env python
from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext

class tiny_build_ext(build_ext):
   def build_extensions(self):
      build_ext.build_extensions(self)

pytiny_ext = Extension('pytiny',
		       sources = ['tinyexpr/tinyexpr.c', 'pytiny.cpp'],
		       extra_compile_args=[],
		       extra_objects=[])

setup(name="pytiny",
      cmdclass={"build_ext": tiny_build_ext,},
      extra_link_args=["-fPIC"],
      version="0.10.0",
      description="Python interface for tinyexpr",
      ext_modules=[pytiny_ext])
