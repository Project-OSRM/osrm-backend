#!/usr/bin/env bash

set -e
set -o pipefail

docker build \
    -t beemo/osrm:linux \
    mydocker/
