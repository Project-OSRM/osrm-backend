#!/usr/bin/env bash

MASON_NAME=ragel
MASON_VERSION=6.9
MASON_LIB_FILE=bin/ragel

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.colm.net/files/ragel/ragel-${MASON_VERSION}.tar.gz \
        adf45ba5bb04359e6a0f8d5a98bfc10e6388bf21

    mason_extract_tar_gz

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
