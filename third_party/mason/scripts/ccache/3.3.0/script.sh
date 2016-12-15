#!/usr/bin/env bash

MASON_NAME=ccache
MASON_VERSION=3.3.0
MASON_LIB_FILE=bin/ccache

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://www.samba.org/ftp/ccache/${MASON_NAME}-3.3.tar.bz2 \
        7b97be7b05a4bec29d0466a4e6199b4ec49eb4ca

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-3.3
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --with-bundled-zlib
    make V=1 -j${MASON_CONCURRENCY}
    make install
}

function mason_ldflags {
    :
}

function mason_cflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
