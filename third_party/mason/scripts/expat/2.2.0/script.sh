#!/usr/bin/env bash

MASON_NAME=expat
MASON_VERSION=2.2.0
MASON_LIB_FILE=lib/libexpat.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/expat.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://downloads.sourceforge.net/project/expat/expat/${MASON_VERSION}/expat-${MASON_VERSION}.tar.bz2 \
        de632147cebfb22e51c8ef35fe0f8badcd424a47

    mason_extract_tar_bz2

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
