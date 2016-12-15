#!/usr/bin/env bash

MASON_NAME=cmake
MASON_VERSION=3.6.2
MASON_LIB_FILE=bin/cmake

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.cmake.org/files/v3.6/cmake-${MASON_VERSION}.tar.gz \
        f2c114944dafb319c27bdca214ca7e0739a71cb0

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    ./configure --prefix=${MASON_PREFIX}
    make -j${MASON_CONCURRENCY} VERBOSE=1
    make install
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
