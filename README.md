
    git clone https://github.com/star-bnl/star-cvs.git
    git clone https://github.com/star-bnl/star-cmake.git
    cd star-cmake
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DSTAR_SRC=`pwd`/../../star-cvs/
    make
