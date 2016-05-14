#!/usr/bin/env bash

set -e
set -o pipefail

export CMAKEOPTIONS="-DCMAKE_BUILD_TYPE=Release"

cd /home/mapbox/osrm-backend
[ -d build ] && rm -rf build
mkdir -p build
cd build
cmake .. $CMAKEOPTIONS -DBUILD_TOOLS=1

make -j`nproc`
make tests -j`nproc`
#./unit_tests/server-tests
#./unit_tests/library-tests
#./unit_tests/extractor-tests
#./unit_tests/util-tests
cd ..
npm test
