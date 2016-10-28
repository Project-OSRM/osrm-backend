#!/usr/bin/env bash

MASON_NAME=jemalloc
MASON_VERSION=4.2.1
MASON_LIB_FILE=bin/jeprof

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/jemalloc/jemalloc/releases/download/${MASON_VERSION}/jemalloc-${MASON_VERSION}.tar.bz2 \
        6bd515d75d192ac82f3171d36a998e0e6e77ac8a

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    # warning: CFLAGS overwrites jemalloc CFLAGS, so we need to add back the jemalloc defaults
    export CFLAGS="${CFLAGS} -std=gnu99 -Wall -pipe -O3 -funroll-loops"
    ./configure --prefix=${MASON_PREFIX}
    make -j${MASON_CONCURRENCY} VERBOSE=1
    make install
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
