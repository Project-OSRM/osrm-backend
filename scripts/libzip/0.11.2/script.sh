#!/usr/bin/env bash

MASON_NAME=libzip
MASON_VERSION=0.11.2
MASON_LIB_FILE=lib/libzip.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libzip.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.nih.at/libzip/libzip-0.11.2.tar.gz \
        5e2407b231390e1cb8234541e89693ae57487170

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libzip-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking

    make install -j${MASON_CONCURRENCY}
}

function mason_strip_ldflags {
    shift # -L...
    shift # -lzip
    echo "$@"
}

function mason_ldflags {
    mason_strip_ldflags $(`mason_pkgconfig` --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"
