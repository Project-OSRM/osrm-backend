#!/usr/bin/env bash

MASON_NAME=zlib
MASON_VERSION=1.2.8
MASON_LIB_FILE=lib/libz.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/zlib.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/madler/zlib/archive/v1.2.8.tar.gz \
        72509ccfd1708e0073c84e8ee09de7a869816823

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/zlib-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        --static

    make install -j${MASON_CONCURRENCY}
}

mason_run "$@"
