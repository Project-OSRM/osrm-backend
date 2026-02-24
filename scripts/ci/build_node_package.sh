#!/bin/bash

set -e -o pipefail

source build/osrm-run.env

echo "node version is:"
which node
node -v

NPM_FLAGS="--directory build/nodejs"
if [[ "$OSRM_CONFIG" == "Debug" ]]; then
    NPM_FLAGS="$NPM_FLAGS --debug"
fi

echo cmake --install "${CMAKE_BINARY_DIR}" --config "${OSRM_CONFIG}" --component node_osrm -v
cmake --install "${CMAKE_BINARY_DIR}" --config "${OSRM_CONFIG}" --component node_osrm -v

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS
./node_modules/.bin/node-pre-gyp package testpackage $NPM_FLAGS
