#!/usr/bin/env bash

MASON_NAME=binutils
MASON_VERSION=2.26
MASON_LIB_FILE=bin/ld

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://ftp.gnu.org/gnu/binutils/${MASON_NAME}-${MASON_VERSION}.tar.bz2 \
        05b22d6ef8003e76f7d05500363a3ee8cc66a7ae

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

    make -j${MASON_CONCURRENCY}
    make install
}

function mason_strip_ldflags {
    shift # -L...
    shift # -lpng16
    echo "$@"
}

function mason_ldflags {
    mason_strip_ldflags $(`mason_pkgconfig` --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"
