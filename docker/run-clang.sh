#!/usr/bin/env bash

set -e
set -o pipefail

docker run \
    -i \
    -e "CXX=clang++" \
    -v `pwd`:/home/mapbox/osrm-backend \
    -t mapbox/osrm:linux \
    osrm-backend/docker/test.sh
