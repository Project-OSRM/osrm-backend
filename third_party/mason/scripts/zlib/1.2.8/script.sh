#!/usr/bin/env bash

MASON_NAME=zlib
MASON_VERSION=1.2.8
MASON_LIB_FILE=lib/libz.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/zlib.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://zlib.net/zlib-1.2.8.tar.gz \
        ed88885bd4027806753656d64006ab86a29e967e

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/zlib-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        --static

    make install -j${MASON_CONCURRENCY}
}

function mason_strip_ldflags {
    shift # -L...
    shift # -lz
    echo "$@"
}

function mason_ldflags {
    mason_strip_ldflags $(`mason_pkgconfig` --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"
