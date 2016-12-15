#!/usr/bin/env bash

MASON_NAME=nasm
MASON_VERSION=2.11.06
MASON_LIB_FILE=bin/nasm

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.nasm.us/pub/nasm/releasebuilds/${MASON_VERSION}/${MASON_NAME}-${MASON_VERSION}.tar.bz2 \
        9602eca86270d4df37f53ae4de2342073ad4adc7

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --with-pic \
        --disable-shared \
        --disable-dependency-tracking

    make install install_rdf -j${MASON_CONCURRENCY}
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
