#!/usr/bin/env bash

MASON_NAME=sqlite
MASON_VERSION=3.9.1
MASON_LIB_FILE=lib/libsqlite3.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/sqlite3.pc

SQLITE_FILE_VERSION=3090100

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://www.sqlite.org/2015/sqlite-autoconf-${SQLITE_FILE_VERSION}.tar.gz \
        92d8a46908e793c9e387f3199699783fd5f6ebed

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/sqlite-autoconf-${SQLITE_FILE_VERSION}
}

function mason_compile {
    CFLAGS="-O3 ${CFLAGS}" ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --with-pic \
        --disable-shared \
        --disable-dependency-tracking

    make install -j${MASON_CONCURRENCY}
}

function mason_strip_ldflags {
    shift # -L...
    shift # -lsqlite3
    echo "$@"
}

function mason_ldflags {
    mason_strip_ldflags $(`mason_pkgconfig` --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"
