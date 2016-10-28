#!/usr/bin/env bash

MASON_NAME=sqlite
MASON_VERSION=3.8.8.1
MASON_LIB_FILE=lib/libsqlite3.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/sqlite3.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.sqlite.org/2015/sqlite-autoconf-3080801.tar.gz \
        24012945241c0b55774b8bad2679912e14703a24

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/sqlite-autoconf-3080801
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
