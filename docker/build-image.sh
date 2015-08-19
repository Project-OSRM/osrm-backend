#!/usr/bin/env bash

set -e
set -o pipefail

docker build \
    -t mapbox/osrm:linux \
    docker/

