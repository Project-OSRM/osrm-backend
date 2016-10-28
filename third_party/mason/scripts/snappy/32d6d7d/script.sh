#!/usr/bin/env bash

MASON_NAME=snappy
MASON_VERSION=32d6d7d
MASON_LIB_FILE=lib/libsnappy.a
MASON_PKGCONFIG_FILE=snappy.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/snappy/archive/32d6d7d8a2ef328a2ee1dd40f072e21f4983ebda.tar.gz \
        2faac1dbc670bffc462cb87a370e6b76371d7764

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-32d6d7d8a2ef328a2ee1dd40f072e21f4983ebda
}

function mason_prepare_compile {
    ./autogen.sh
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking

    make install -j${MASON_CONCURRENCY}
}

function mason_clean {
    make clean
}

mason_run "$@"
