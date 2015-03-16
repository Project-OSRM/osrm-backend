#!/usr/bin/env bash

set -e
set -o pipefail

docker build \
    -t mapbox/osrm:linux \
    docker/

docker run \
    -i \
    -e "CXX=g++" \
    -v `pwd`:/home/mapbox/osrm-backend \
    -t mapbox/osrm:linux \
    osrm-backend/docker/test.sh
