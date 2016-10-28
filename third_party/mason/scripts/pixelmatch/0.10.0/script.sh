#!/usr/bin/env bash

MASON_NAME=pixelmatch
MASON_VERSION=0.10.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
    https://github.com/mapbox/pixelmatch-cpp/archive/v${MASON_VERSION}.tar.gz \
    cfcacf7e910c43ee0570bd069c42026095bf28ae
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/pixelmatch-cpp-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include
    cp -rv include ${MASON_PREFIX}
    cp -v README.md LICENSE.txt ${MASON_PREFIX}
}

function mason_cflags {
    echo -isystem ${MASON_PREFIX}/include -I${MASON_PREFIX}/include
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

mason_run "$@"
