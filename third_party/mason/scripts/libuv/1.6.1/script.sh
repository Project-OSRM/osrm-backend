#!/usr/bin/env bash

MASON_NAME=libuv
MASON_VERSION=1.6.1
MASON_LIB_FILE=lib/libuv.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libuv.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/libuv/libuv/archive/v1.6.1.tar.gz \
        7e0bf128a4e7e82f8d3a13bf789e66ad963f82a1

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libuv-${MASON_VERSION}
}

function mason_prepare_compile {
    ./autogen.sh
}

function mason_compile {
    CFLAGS="-O3 -DNDEBUG -fPIC ${CFLAGS}" ./configure \
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
