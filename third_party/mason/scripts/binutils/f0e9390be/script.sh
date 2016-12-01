#!/usr/bin/env bash

MASON_NAME=binutils
MASON_VERSION=f0e9390be
MASON_LIB_FILE=bin/ld

. ${MASON_DIR}/mason.sh

function mason_load_source {
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
    git clone git://sourceware.org/git/binutils-gdb.git ${MASON_BUILD_PATH}
    cd ${MASON_BUILD_PATH}
    git checkout f0e9390be5bbfa3ee777d81dacfccd713ebddb68
    cd ../
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
        --disable-werror \
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
