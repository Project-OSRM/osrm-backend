#!/usr/bin/env bash

MASON_NAME=sqlite
MASON_VERSION=3.16.2
MASON_LIB_FILE=lib/libsqlite3.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/sqlite3.pc

SQLITE_FILE_VERSION=3160200

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://www.sqlite.org/2017/sqlite-autoconf-${SQLITE_FILE_VERSION}.tar.gz \
        e070d08277ed4f1da54a0d31d85cda2a14438e3f

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/sqlite-autoconf-${SQLITE_FILE_VERSION}
}

function mason_compile {
    # Note: setting CFLAGS overrides the default in sqlite of `-g -O2`
    # hence we add back the preferred optimization
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
