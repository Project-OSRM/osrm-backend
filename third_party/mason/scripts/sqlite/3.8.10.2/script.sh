#!/usr/bin/env bash

MASON_NAME=sqlite
MASON_VERSION=3.8.10.2
MASON_LIB_FILE=lib/libsqlite3.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/sqlite3.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://www.sqlite.org/2015/sqlite-autoconf-3081002.tar.gz \
        841053dd606523187b42aa2dd8d9356735d3d382

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/sqlite-autoconf-3081002
}

function mason_compile {
    CFLAGS="-O3 ${CFLAGS}" ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --with-pic \
        --disable-shared \
        --disable-dependency-tracking \
        --disable-dynamic-extensions

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
