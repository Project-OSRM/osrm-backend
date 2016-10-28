#!/usr/bin/env bash

set -e -u
set -o pipefail

# ensure building a C++ lib works
./mason build stxxl 1.4.1

# ensure linking results in expected files
./mason link stxxl 1.4.1

failure=0

if [[ ! -f mason_packages/.link/lib/libstxxl.a ]]; then
    echo "could not find expected lib/libstxxl.a"
    failure=1
fi

if [[ ! -f mason_packages/.link/lib/pkgconfig/stxxl.pc ]]; then
    echo "could not find expected lib/pkgconfig/stxxl.pc"
    failure=1
fi

if [[ ! -d mason_packages/.link/include/stxxl ]]; then
    echo "could not find expected include/stxxl"
    failure=1
fi

if [[ ! -f mason_packages/.link/include/stxxl.h ]]; then
    echo "could not find expected include/stxxl.h"
    failure=1
fi

exit $failure