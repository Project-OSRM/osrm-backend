#!/usr/bin/env bash

set -e
set -o pipefail

docker run \
    -i \
    -e "CXX=clang++" \
    -v `pwd`:/home/mapbox/osrm-backend \
    -t mapbox/osrm:linux \
    /bin/bash -lc "osrm-backend/docker/test.sh"
