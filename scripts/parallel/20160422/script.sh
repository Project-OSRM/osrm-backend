#!/usr/bin/env bash

MASON_NAME=parallel
MASON_VERSION=20160422
MASON_LIB_FILE=bin/parallel

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://ftp.gnu.org/gnu/${MASON_NAME}/${MASON_NAME}-${MASON_VERSION}.tar.bz2 \
        032c35aaecc65aa1298b33c48f0a4418041771e4

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    ./configure --prefix=${MASON_PREFIX} \
     --disable-dependency-tracking

    make -j${MASON_CONCURRENCY}
    make install
}


function mason_clean {
    make clean
}

mason_run "$@"
