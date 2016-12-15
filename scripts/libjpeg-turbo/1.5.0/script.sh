#!/usr/bin/env bash

MASON_NAME=libjpeg-turbo
MASON_VERSION=1.5.0
MASON_LIB_FILE=lib/libjpeg.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/libjpeg-turbo/libjpeg-turbo/archive/1.5.0.tar.gz \
        8b06f41d821a4dbcd20fd5fea97ba80916a22b00

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libjpeg-turbo-1.5.0
}


function mason_compile {
    export CFLAGS="${CFLAGS:-} -O3"
    autoreconf -fiv
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --with-jpeg8 \
        --without-turbojpeg \
        --enable-static \
        --with-pic \
        --disable-shared \
        --disable-dependency-tracking \
        NASM=yasm

    V=1 make install -j${MASON_CONCURRENCY}
    rm -rf ${MASON_PREFIX}/bin
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    : # We're only using the full path to the archive, which is output in static_libs
}

function mason_clean {
    make clean
}

mason_run "$@"
