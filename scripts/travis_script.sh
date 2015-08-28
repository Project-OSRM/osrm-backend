#!/bin/sh
#
#  travis_script.sh
#

mkdir build
cd build

# GCC ignores the pragmas in the code that disable the "return-type" warning
# selectively, so use this workaround.
if [ "${CXX}" = "g++" ]; then
    WORKAROUND="-DCMAKE_CXX_FLAGS=-Wno-return-type"
else
    WORKAROUND=""
fi

if [ "${CXX}" = "g++" ]; then
    CXX=g++-4.8
    CC=gcc-4.8
fi

cmake -LA \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    ${WORKAROUND} \
    ..

make VERBOSE=1
ctest --output-on-failure

