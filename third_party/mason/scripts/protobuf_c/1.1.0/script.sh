#!/usr/bin/env bash

MASON_NAME=protobuf_c
MASON_VERSION=1.1.0
MASON_LIB_FILE=lib/libprotobuf-c.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/protobuf-c.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/protobuf-c/protobuf-c/releases/download/v1.1.0/protobuf-c-1.1.0.tar.gz \
        c7e30c4410e50a14f9eeffbc26dbc62ab4d3ebc5

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/protobuf-c-${MASON_VERSION}
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install protobuf 2.6.1
    MASON_PROTOBUF=$(${MASON_DIR}/mason prefix protobuf 2.6.1)
    export PKG_CONFIG_PATH=${MASON_PROTOBUF}/lib/pkgconfig:${PKG_CONFIG_PATH}
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
