#!/usr/bin/env bash

set -eu
set -o pipefail

packages=(llvm clang++ clang-tidy clang-format lldb llvm-cov include-what-you-use)
versions=(3.9.1 4.0.0 3.8.1)

if [[ $(uname -s) == 'Linux' ]]; then
    CLANG_BOOTSTRAP_VERSION="3.8.1"
    ./mason install clang++ ${CLANG_BOOTSTRAP_VERSION}
    CLANG_PREFIX=$(./mason prefix clang++ ${CLANG_BOOTSTRAP_VERSION})
    export CXX=${CLANG_PREFIX}/bin/clang++
    export CC=${CLANG_PREFIX}/bin/clang
fi

function build() {
    local VERSION=$1
    for package in "${!packages[@]}"; do
        ./mason build ${packages[$package]} ${VERSION}
    done
}

function publish() {
    local VERSION=$1
    for package in "${!packages[@]}"; do
        ./mason publish ${packages[$package]} ${VERSION}
    done
}

function new_version() {
    local NEW_VERSION="$1"
    local LAST_VERSION="$2"
    for package in "${!packages[@]}"; do
        mkdir -p scripts/${package}/${NEW_VERSION}
        cp -r scripts/${package}/${LAST_VERSION}/. scripts/${package}/${NEW_VERSION}/
    done
}


function build_all() {
    for ver in "${!versions[@]}"; do
        build ${versions[$ver]}
        publish ${versions[$ver]}
    done
}

if [[ ${1:-0} == "all" ]]; then
    build_all
fi
