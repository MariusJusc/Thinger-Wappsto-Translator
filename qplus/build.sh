#!/bin/sh

ABORT=no

check_cmake()
  {
  (cmake --version) </dev/null >/dev/null 2>&1 ||
    {
    echo "error: cmake 2.6 must be present to configure and install qplus"
    echo ""
    echo "cmake might be available as a package for your system,"
    echo "or can be downloaded from http://cmake.org"
    ABORT=yes
    }
  }

check_cmake

test "$ABORT" = yes && exit -1

BUILD_DIR="build"

if [ ! -d  ${BUILD_DIR} ]; then
    mkdir ${BUILD_DIR} 
    cd ${BUILD_DIR} 
else
    cd ${BUILD_DIR} 
fi

rm -f CMakeCache.txt
cmake -DCMAKE_INSTALL_PREFIX=../ ../
make
#make install
