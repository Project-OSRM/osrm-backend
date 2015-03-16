#!/usr/bin/env bash

set -e
set -o pipefail

export TRAVIS_OS_NAME=linux
export CMAKEOPTIONS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-4.8"
export OSRM_PORT=5000
export OSRM_TIMEOUT=60
export PATH=$PATH:/home/mapbox/.gem/ruby/1.9.1/bin:/home/mapbox/osrm-backend/vendor/bundle/ruby/1.9.1/bin
export BRANCH=develop

cd /home/mapbox/osrm-backend
gem install --user-install bundler
bundle install --path vendor/bundle
[ -d build ] && rm -rf build
mkdir -p build
cd build
cmake .. $CMAKEOPTIONS -DBUILD_TOOLS=1

make -j`nproc`
make tests -j`nproc`
./datastructure-tests
./algorithm-tests
cd ..
bundle exec cucumber -p verify
