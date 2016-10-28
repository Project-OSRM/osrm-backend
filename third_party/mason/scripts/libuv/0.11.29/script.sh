#!/usr/bin/env bash

MASON_NAME=libuv
MASON_VERSION=0.11.29
MASON_LIB_FILE=lib/libuv.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libuv.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/joyent/libuv/archive/v0.11.29.tar.gz \
        5bf49a8652f680557cbaf335a160187b2da3bf7f

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libuv-${MASON_VERSION}
}

function mason_prepare_compile {
    ./autogen.sh
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking \
        --disable-dtrace

    make install -j${MASON_CONCURRENCY}
}

function mason_strip_ldflags {
    shift # -L...
    shift # -luv
    echo "$@"
}

function mason_ldflags {
    mason_strip_ldflags $(`mason_pkgconfig` --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"
