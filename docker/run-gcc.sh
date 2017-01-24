#!/usr/bin/env bash

set -e
set -o pipefail

docker run \
    -i \
    -e "CXX=g++-5" \
    -e "CC=gcc-5" \
    -v `pwd`:/home/mapbox/osrm-backend \
    -t mapbox/osrm:linux \
    /bin/bash -lc "osrm-backend/docker/test.sh"
