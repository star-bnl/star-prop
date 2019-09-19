The [software used by the STAR experiment](https://github.com/star-bnl/star-sw)
is based on more than 20 years of effort invested by scientists and students
with various levels of experience and knowledge in software development.
Historically, the software was built using the GNU
[`cons`](https://www.gnu.org/software/cons/) build system. This project
provides an alternative way to build the libraries based on the
[CMake](https://cmake.org/) family of tools. Below we highlight a few aspects
motivated this work.

- According to `cons` web page (last updated in 2001)
  > cons has been decommissioned [...]

  Meanwhile, CMake is close to becoming a "standard" build tool for C++
  projects

- Unlike with `cons` multi-threaded builds are possible with CMake (e.g. `make -j`).
  Our tests indicate a reduction in build time from 2 hours to under 20 minutes
  of the entire code base using four concurrent threads

- With `cons` builds the separation of the source, build, and install locations
  is not well defined. Our CMake builds allow one to install the libraries and
  auxiliary files into *well-isolated* multiple directories from same source

We also provide [Dockerfiles](docker/README.md) for anyone to use as an example
for building a reproducible working environment or to inspect for all packages
required by the STAR software.


# How to build and install STAR libraries

## Prerequisites

Several external tools required to build the STAR libraries on a Linux box:

- CMake (>= 3.6)
- C++ compiler with C++11 support (gcc >= 4.8.5)
- Fortran compiler (gcc >= 4.8.5)
- bison 2.7 or yacc 1.9
- flex 2.5
- perl 5.16
- python2, pip, pyparsing, and standard linux tools such as awk, grep, ...

The STAR software also depends on the following external packages/libraries:

- ROOT5 (e.g. 5.34.38), http://root.cern
- MySQL
- XML2
- X11, log4cxx, curl
- CERNLIB, lapack, blas

## Build with CMake and make

    git clone https://gitlab.com/star-bnl/star-sw.git
    git clone https://gitlab.com/star-bnl/star-cvs.git
    mkdir star-build
    cd star-build
    cmake ../star-sw -DSTAR_SRC=../star-cvs/ -DCMAKE_INSTALL_PREFIX=../star-install
    make -j[N]
    make install

where `[N]` is the number of compile jobs to run simultaneously, e.g. set it to`$(nproc)`.


# Release Notes

Highlighted features of the past and planned releases.

__Next proposed features__

- Revisit `cvs2git` scripts and move them to this repository
- Implement basic CI based on STAR nightly tests

__0.1.0__

- Support build of all STAR libraries needed to run reconstruction and embedding
  jobs
