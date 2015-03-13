#!/usr/bin/env bash

set -e
set -o pipefail

docker build \
    -t mapbox/osrm:linux \
    docker/

docker run \
    -i \
    -e "CXX=g++" \
    -v `pwd`:/home/mapbox/build \
    -t mapbox/osrm:linux \
    build/docker/test.sh
