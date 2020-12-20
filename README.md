Spelling Checker for C++ Source Code
====================================

Introduction
------------

The tools in this package help check the spelling of identifiers, literals and
comments in C++ source code (including headers).


Status
------

Early prototype.


Dependencies
------------

LLVM toolchain (tested with version 11)

CMake 3.18.0 (or later)

CXXOPTS: https://github.com/jarro2783/cxxopts

fmtlib: https://github.com/fmtlib/fmt

No package management is employed at this time, the build scripts use the
packages as installed on Debian testing.


Acknowledgments
---------------

Project started due to nerd-sniping by Ken Chalmers (@kchalmer) who found a
similar implementation in Python
(https://github.com/louis-langholtz/pyspellcode) but that tool is seems has
not received any updates in the past few years.


License
-------

Copyright 2020 Florin Iucha

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
