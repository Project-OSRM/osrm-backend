#!/bin/sh

set -e

CMAKE_OPTIONS=${CMAKE_OPTIONS:-"-DCMAKE_BUILD_TYPE=Release -DENABLE_NODE_BINDINGS=On -DENABLE_MASON=On"}

if [[ ! -f $(which cmake) ]]; then
    echo "Needs cmake to build from source"
    exit 1
fi
if [[ ! -f $(which clang) ]]; then
    echo "Needs clang to build from source"
    exit 1
fi
if [[ ! -f $(which clang++) ]]; then
    echo "Needs clang++ to build from source"
    exit 1
fi

if [[ -d build ]]; then
    echo "Detected existing build directory, skipping compiling."
    exit 0
else
    mkdir -p build
    pushd build
    CXX=clang++ CC=clang cmake .. $CMAKE_OPTIONS
    popd
fi
