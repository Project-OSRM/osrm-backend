#!/usr/bin/env bash

set -e -u
set -o pipefail

failure=0

if [[ ! -d mason_packages/.link/include/stxxl ]]; then
    echo "could not find expected include/stxxl"
    failure=1
fi

if [[ ! -L mason_packages/.link/include/stxxl ]]; then
    echo "include/stxxl is expected to be a symlink"
    failure=1
fi

if [[ $(uname -s) == 'Darwin' ]]; then
    if [[ ! -f mason_packages/.link/lib/libstxxl.dylib ]]; then
        echo "libstxxl.dylib expected to be present"
        failure=1
    fi
elif [[ $(uname -s) == 'Linux' ]]; then
    if [[ ! -f mason_packages/.link/lib/libstxxl.so ]]; then
        echo "libstxxl.dylib expected to be present"
        failure=1
    fi
fi

exit $failure

