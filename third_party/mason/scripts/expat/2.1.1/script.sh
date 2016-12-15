#!/usr/bin/env bash

MASON_NAME=expat
MASON_VERSION=2.1.1
MASON_LIB_FILE=lib/libexpat.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/expat.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://fossies.org/linux/www/expat-${MASON_VERSION}.tar.gz \
        b939d8395c87b077f3562fca0096c03728b71e1f

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
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


function mason_clean {
    make clean
}

mason_run "$@"
