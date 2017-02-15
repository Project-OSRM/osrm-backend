#!/usr/bin/env bash

MASON_NAME=jemalloc
MASON_VERSION=4.4.0
MASON_LIB_FILE=bin/jeprof

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/jemalloc/jemalloc/releases/download/${MASON_VERSION}/jemalloc-${MASON_VERSION}.tar.bz2 \
        90f752aeb070639f5f8fd5d87a86cf9cc2ddc8f3

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    # warning: CFLAGS overwrites jemalloc CFLAGS, so we need to add back the jemalloc defaults
    export CFLAGS="${CFLAGS} -std=gnu11 -Wall -pipe -O3 -funroll-loops -DNDEBUG -D_REENTRANT"
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
