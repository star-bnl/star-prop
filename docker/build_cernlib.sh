#!/bin/bash

# Establish the environment variables for the build procedures
# Depending on the system, other directories may need to be added to the PATH
# e.g. for the build tools and alternative compilers.

CERN_LEVEL=2006

CERN=`pwd`
CERN_ROOT=$CERN/$CERN_LEVEL
CERN_INCLUDEDIR=$CERN/$CERN_LEVEL/include
CVSCOSRC=$CERN/$CERN_LEVEL/src
PATH=$CERN_ROOT/bin:$PATH

export CERN
export CERN_LEVEL
export CERN_ROOT
export CERN_INCLUDEDIR
export CVSCOSRC
export PATH

# Create the build directory structure

cd $CERN_ROOT
rm -fr build bin lib include
mkdir -p build bin lib log include

# Create the top level Makefile with imake

cd $CERN_ROOT/build
$CVSCOSRC/config/imake_boot

# Install kuipc and the scripts (cernlib, paw and gxint) in $CERN_ROOT/bin

make bin/kuipc &> ../log/kuipc
make scripts/Makefile
cd scripts
make install.bin &> ../../log/scripts

# Install the libraries

cd $CERN_ROOT/build
make &> ../log/make.`date +%m%d`
make install.include &> ../log/make_install.`date +%m%d`
