#!/usr/bin/env bash

MASON_NAME=protobuf
MASON_VERSION=2.6.1
MASON_LIB_FILE=lib/libprotobuf-lite.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/protobuf-lite.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.bz2 \
        823368b46eee836243c20f0c358b15280b0f8cf9

    mason_extract_tar_bz2

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
