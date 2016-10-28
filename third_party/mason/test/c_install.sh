#!/usr/bin/env bash

set -e -u
set -o pipefail

./mason install sqlite 3.8.8.1
./mason install libuv 0.10.28
./mason install libuv 0.11.29
