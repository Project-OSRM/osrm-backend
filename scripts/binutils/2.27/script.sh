#!/usr/bin/env bash

MASON_NAME=binutils
MASON_VERSION=2.27
MASON_LIB_FILE=bin/ld

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://ftp.gnu.org/gnu/binutils/${MASON_NAME}-${MASON_VERSION}.tar.bz2 \
        7e62c56ea660080882af2c8644d566765a77a0b8

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    # we unset CFLAGS otherwise they will clobber defaults inside binutils
    unset CFLAGS
    ./configure \
        --prefix=${MASON_PREFIX} \
        --enable-gold \
        --enable-plugins \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking

    make -j${MASON_CONCURRENCY}
    make install
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
