#!/usr/bin/env bash

MASON_NAME=node_asan
MASON_VERSION=4.4.4
MASON_LIB_FILE=bin/node

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://nodejs.org/dist/v${MASON_VERSION}/node-v${MASON_VERSION}.tar.gz \
        71c6b67274b5e042366e7fbba1ee92426ce6fe1a

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/node-v${MASON_VERSION}
}

function mason_compile {
    export CXXFLAGS="${CXXFLAGS} -fsanitize=address"
    export CFLAGS="${CFLAGS} -fsanitize=address"
    export LDFLAGS="${LDFLAGS} -fsanitize=address"
    if [[ $(uname -s) == 'Darwin' ]]; then
        export CXXFLAGS="${CXXFLAGS} -std=c++11 -stdlib=libc++"
        export LDFLAGS="${LDFLAGS} -std=c++11 -stdlib=libc++"
    fi

    ./configure \
        --prefix=${MASON_PREFIX} \
        --debug
    echo "making binary"
    make binary -j${MASON_CONCURRENCY}
    ls
    echo "uncompressing binary"
    tar -xf *.tar.gz
    echo "making dir"
    mkdir -p ${MASON_PREFIX}
    echo "making copying"
    cp -r node-v${MASON_VERSION}*/* ${MASON_PREFIX}/
}

function mason_clean {
    make clean
}

mason_run "$@"
