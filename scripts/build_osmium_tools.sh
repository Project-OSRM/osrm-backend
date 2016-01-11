#!/usr/bin/env bash

# Builds command line tools shipped with libosmium for example osmium_convert
# CMake build directory is build/osmium; binaries are located under build/osmium/examples


# e: exit on first error, x: print commands
set -ex

BUILD_DIR=build/osmium

cmake -E remove_directory $BUILD_DIR
cmake -E make_directory $BUILD_DIR
cmake -E chdir $BUILD_DIR cmake ../../third_party/libosmium -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=1 -DBUILD_TESTING=0
cmake -E chdir $BUILD_DIR cmake --build .
