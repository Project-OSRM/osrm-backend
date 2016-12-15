#!/usr/bin/env bash

set -e -u
set -o pipefail

./mason install libpng 1.6.16
./mason link libpng 1.6.16

failure=0

if [[ ! -d mason_packages/.link/include/libpng16 ]]; then
    echo "could not find expected include/libpng16"
    failure=1
fi

if [[ ! -L mason_packages/.link/include/libpng16/png.h ]]; then
    echo "include/libpng16/png.h is expected to be a symlink"
    failure=1
fi

if [[ ! -L mason_packages/.link/include/png.h ]]; then
    echo "include/png.h is expected to be a symlink"
    failure=1
fi

exit $failure

