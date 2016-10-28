#!/usr/bin/env bash

MASON_NAME=expat
MASON_VERSION=2.1.0
MASON_LIB_FILE=lib/libexpat.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/expat.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://mapnik.s3.amazonaws.com/deps/expat-2.1.0.tar.gz \
        a791223d3b20ab54e59ed0815afd2ef2fb81194e

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
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
