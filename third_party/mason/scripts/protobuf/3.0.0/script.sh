#!/usr/bin/env bash

MASON_NAME=protobuf
MASON_VERSION=3.0.0
MASON_LIB_FILE=lib/libprotobuf.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/protobuf.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/${MASON_NAME}/releases/download/v${MASON_VERSION}/${MASON_NAME}-cpp-${MASON_VERSION}.tar.gz \
        88f03530236797ee1a51748cb9a1fe43cc952b7c

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static --disable-shared \
        --disable-debug --without-zlib \
        --disable-dependency-tracking

    make install -j${MASON_CONCURRENCY}
}

function mason_clean {
    make clean
}

mason_run "$@"

