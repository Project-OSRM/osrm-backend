#!/usr/bin/env bash

set -e -u
set -o pipefail

./mason install boost_libregex 1.57.0
./mason link boost_libregex 1.57.0


failure=0

if [[ ! -f mason_packages/.link/lib/libboost_regex.a ]]; then
    echo "could not find expected lib/libboost_regex.a"
    failure=1
fi

if [[ ! -L mason_packages/.link/lib/libboost_regex.a ]]; then
    echo "lib/libboost_regex.a is not a symlink like expected"
    failure=1
fi

exit $failure