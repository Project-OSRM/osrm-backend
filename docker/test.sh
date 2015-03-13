#!/usr/bin/env bash

set -e
set -o pipefail

export TRAVIS_OS_NAME=linux
export CMAKEOPTIONS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-4.8"
export OSRM_PORT=5000
export OSRM_TIMEOUT=60

rvm use 1.9.3
gem install bundler
bundle install

mkdir -p build
cd build
cmake .. $CMAKEOPTIONS -DBUILD_TOOLS=1

make -j`nproc`
make tests -j`nproc`
./datastructure-tests
./algorithm-tests
cd ..
cucumber -p verify
