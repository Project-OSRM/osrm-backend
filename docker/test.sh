#!/usr/bin/env bash

set -e
set -o pipefail

export CMAKEOPTIONS="-DCMAKE_BUILD_TYPE=Release"
export PATH=$PATH:/home/mapbox/.gem/ruby/1.9.1/bin:/home/mapbox/osrm-backend/vendor/bundle/ruby/1.9.1/bin

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
