#!/usr/bin/env bash

MASON_NAME=sqlite
MASON_VERSION=3.8.8.3
MASON_LIB_FILE=lib/libsqlite3.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/sqlite3.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://sqlite.org/2015/sqlite-autoconf-3080803.tar.gz \
        55d0c095e5bf76ed7b450265261b367228bbd0ba

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/sqlite-autoconf-3080803
}

function mason_compile {
    ./configure \
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
