#!/usr/bin/env bash

set -eu
set -o pipefail

CLANG_VERSION="3.9.1"
./mason install clang++ ${CLANG_VERSION}
export PATH=$(./mason prefix clang++ ${CLANG_VERSION})/bin:${PATH}
export CXX=clang++-3.9
export CC=clang-3.9

set +eu