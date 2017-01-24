#!/usr/bin/env bash

MASON_NAME=pkgconfig
MASON_VERSION=0.29.1
MASON_LIB_FILE=bin/pkg-config

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://pkgconfig.freedesktop.org/releases/pkg-config-${MASON_VERSION}.tar.gz \
        e331df90bfd646f7b2ca1d66fb3c4fd74f4ec11c

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/pkg-config-${MASON_VERSION}
}

function mason_compile {
    ./configure --prefix=${MASON_PREFIX} ${MASON_HOST_ARG} \
        --disable-debug \
        --with-internal-glib \
        --disable-dependency-tracking \
        --with-pc-path=${MASON_PREFIX}/lib/pkgconfig
    make -j${MASON_CONCURRENCY}
    make install
}

function mason_clean {
    make clean
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

mason_run "$@"
