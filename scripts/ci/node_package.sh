#!/bin/bash

set -eu
set -o pipefail

echo "node version is:"
which node
node -v

NPM_FLAGS=''
if [[ ${BUILD_TYPE} == "Debug" ]]; then
    NPM_FLAGS='--debug'
fi

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS

# enforce that binary has proper ORIGIN flags so that
# it can portably find libtbb.so in the same directory
if [[ $(uname -s) == 'Linux' ]]; then
    readelf -d ./lib/binding/node_osrm.node > readelf-output.txt
    if grep -q 'Flags: ORIGIN' readelf-output.txt; then
        echo "Found ORIGIN flag in readelf output"
        cat readelf-output.txt
    else
        echo "*** Error: Could not found ORIGIN flag in readelf output"
        cat readelf-output.txt
        exit 1
    fi
fi

./node_modules/.bin/node-pre-gyp package testpackage $NPM_FLAGS
