#!/usr/bin/env bash

MASON_NAME=libzip
MASON_VERSION=1.1.3
MASON_LIB_FILE=lib/libzip.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libzip.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.nih.at/libzip/libzip-${MASON_VERSION}.tar.gz \
        4bc18317d0607d5a24b618a6a5c1c229dade48e8

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
