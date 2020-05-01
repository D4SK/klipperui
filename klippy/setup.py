#!/usr/bin/env python3
from setuptools import setup
from Cython.Build import cythonize

setup(ext_modules = cythonize("./*.pyx", compiler_directives={'language_level': "3"}))


# usage:
# source klippy-environment/bin/activate
# ./setup.py build_ext --inplace
