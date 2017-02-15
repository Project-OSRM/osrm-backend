#!/usr/bin/env bash

MASON_NAME=jpeg_turbo
MASON_VERSION=1.5.1
MASON_LIB_FILE=lib/libjpeg.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/libjpeg-turbo/${MASON_VERSION}/libjpeg-turbo-${MASON_VERSION}.tar.gz \
        4038bb4242a3fc3387d5dc4e37fc2ac7fffaf5da

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libjpeg-turbo-${MASON_VERSION}
}

function mason_prepare_compile {
    MASON_PLATFORM= ${MASON_DIR}/mason install nasm 2.11.06
    MASON_NASM=$(MASON_PLATFORM= ${MASON_DIR}/mason prefix nasm 2.11.06)
}

function mason_compile {
    # note CFLAGS overrides defaults so we need to add optimization flags back
    export CFLAGS="${CFLAGS} -O3 -DNDEBUG"
    ./configure \
        NASM="${MASON_NASM}/bin/nasm" \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --with-jpeg8 \
        --without-12bit \
        --without-java \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking

    make V=1 -j1 # -j1 since build breaks with concurrency
    make install
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    echo -L${MASON_PREFIX}/lib -ljpeg
}

function mason_clean {
    make clean
}

mason_run "$@"
