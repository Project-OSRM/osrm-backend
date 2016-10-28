#!/usr/bin/env bash

set -e -u
set -o pipefail

./mason build sqlite 3.8.8.1
./mason build libuv 0.10.28
./mason build libuv 0.11.29