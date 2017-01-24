#!/usr/bin/env bash

MASON_NAME=libjpeg-turbo
MASON_VERSION=1.4.2
MASON_LIB_FILE=lib/libjpeg.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/libjpeg-turbo/1.4.2/libjpeg-turbo-1.4.2.tar.gz \
        d4638b2261ac3c1c20a2a2e1f8e19fc1f11bf524

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libjpeg-turbo-1.4.2
}


function mason_compile {
    export CFLAGS="${CFLAGS:-} -O3"
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
