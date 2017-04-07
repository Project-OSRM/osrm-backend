#!/bin/bash

set -e

CMAKE_OPTIONS=${CMAKE_OPTIONS:-"-DCMAKE_BUILD_TYPE=Release -DENABLE_NODE_BINDINGS=On"}

if [[ ! -f $(which cmake) ]]; then
    echo "Needs cmake to build from source"
    exit 1
fi
if [[ -d build ]]; then
    echo "Detected existing build directory, skipping compiling."
    exit 0
else
    mkdir -p build
    pushd build
    cmake .. $CMAKE_OPTIONS
    cmake --build .
    popd
fi
