#!/usr/bin/env bash

export PATH=/star/u/smirnovd/usr/local/bin/python3:$PATH
export STAR_VERSION=SL19e
export STAR_INSTALL_PREFIX=/gpfs01/star/pwg/smirnovd
export EIGEN_CMAKE_DIR=/gpfs01/star/pwg/smirnovd/eigen-67e894c6cd8f-install/share/eigen3/cmake

git clone https://github.com/star-bnl/star-sw.git
pushd star-sw
git checkout v0.4.2
popd

git clone https://github.com/star-bnl/star-cvs.git
pushd star-cvs
git checkout ${STAR_VERSION}
popd

# 32 bit
export STAR_HOST_SYS=sl74_gcc485
export OPTSTAR=/opt/star/${STAR_HOST_SYS}
export CERN_ROOT=/cern/2006
source /afs/rhic.bnl.gov/star/ROOT/5.34.38/.${STAR_HOST_SYS}/rootdeb/bin/thisroot.sh

# Release
mkdir -p star-build-32-Release && pushd star-build-32-Release
cmake3 ../star-sw -DSTAR_SRC=../star-cvs -DCMAKE_INSTALL_PREFIX=${STAR_INSTALL_PREFIX}/star-install-${STAR_VERSION}-32-Release -DCERNLIB_ROOT=/cern/2006b -DCMAKE_BUILD_TYPE=Release -DEigen3_DIR=$EIGEN_CMAKE_DIR
make -j $(nproc)
make install
popd

# Debug
mkdir -p star-build-32-Debug && pushd star-build-32-Debug
cmake3 ../star-sw -DSTAR_SRC=../star-cvs -DCMAKE_INSTALL_PREFIX=${STAR_INSTALL_PREFIX}/star-install-${STAR_VERSION}-32-Debug -DCERNLIB_ROOT=/cern/2006b -DCMAKE_BUILD_TYPE=Debug -DEigen3_DIR=$EIGEN_CMAKE_DIR
make -j $(nproc)
make install
popd


# 64 bit
export STAR_HOST_SYS=sl74_x8664_gcc485
export OPTSTAR=/opt/star/${STAR_HOST_SYS}
export CERN_ROOT=/cern64/2005
source /afs/rhic.bnl.gov/star/ROOT/5.34.38/.${STAR_HOST_SYS}/rootdeb/bin/thisroot.sh

# Release
mkdir -p star-build-64-Release && pushd star-build-64-Release
cmake3 ../star-sw -DSTAR_SRC=../star-cvs -DCMAKE_INSTALL_PREFIX=${STAR_INSTALL_PREFIX}/star-install-${STAR_VERSION}-64-Release -DCERNLIB_ROOT=/cern64/2005 -DCMAKE_BUILD_TYPE=Release -DEigen3_DIR=$EIGEN_CMAKE_DIR
make -j $(nproc)
make install
popd

# Debug
mkdir -p star-build-64-Debug && pushd star-build-64-Debug
cmake3 ../star-sw -DSTAR_SRC=../star-cvs -DCMAKE_INSTALL_PREFIX=${STAR_INSTALL_PREFIX}/star-install-${STAR_VERSION}-64-Debug -DCERNLIB_ROOT=/cern64/2005 -DCMAKE_BUILD_TYPE=Debug -DEigen3_DIR=$EIGEN_CMAKE_DIR
make -j $(nproc)
make install
popd
