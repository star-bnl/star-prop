How to build and install STAR libraries
=======================================

Prerequisites
-------------

The tools required to build the libraries are:

- cmake (>= 3.2)
- C++ compiler with C++11 support (e.g. g++ >= 4.8.5)
- Fortran compiler (e.g. gfortran >= 4.8.5)

The STAR software depends on the following external packages/libraries:

- ROOT (>= 5.34.30), http://root.cern.ch
- XML2
- MySQL


Build with cmake and make
-------------------------

    git clone https://github.com/star-bnl/star-cvs.git
    git clone https://github.com/star-bnl/star-cmake.git
    mkdir star-build && cd star-build
    cmake ../star-cmake -DSTAR_SRC=../star-cvs/ -DCMAKE_INSTALL_PREFIX=../star-install
    make -j [N]
    make install

where [N] is the number of jobs to run simultaneously
