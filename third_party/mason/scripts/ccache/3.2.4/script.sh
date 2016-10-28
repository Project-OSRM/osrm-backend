#!/usr/bin/env bash

MASON_NAME=ccache
MASON_VERSION=3.2.4
MASON_LIB_FILE=bin/ccache

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://www.samba.org/ftp/ccache/${MASON_NAME}-${MASON_VERSION}.tar.bz2 \
        80bc058b45efdb00ad49de2198047917016e4c29

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --with-bundled-zlib \
        --disable-dependency-tracking
    make V=1 -j${MASON_CONCURRENCY}
    make install
}

function mason_clean {
    make clean
}

mason_run "$@"
