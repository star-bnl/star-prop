How to build and install STAR libraries
=======================================

Prerequisites
-------------

Several external tools required to build the STAR libraries are listed below
along with their minimal version numbers:

- cmake 3.2
- C++ compiler with C++11 support (e.g. g++ 4.8.5)
- Fortran compiler (e.g. gfortran 4.8.5)
- bison 2.7 or yacc 1.9
- flex 2.5

The STAR software also depends on the following external packages/libraries:

- ROOT (>= 5.34.30), http://root.cern.ch
- XML2
- MySQL


Build with cmake and make
-------------------------

    git clone https://github.com/star-bnl/star-cvs.git
    git clone https://github.com/star-bnl/star-cmake.git
    mkdir star-build && cd star-build
    cmake ../star-cmake -DSTAR_SRC=../star-cvs/ -DCMAKE_INSTALL_PREFIX=../star-install -DLOG4CXX_ROOT=$OPTSTAR
    make -j [N]
    make install

where [N] is the number of jobs to run simultaneously
