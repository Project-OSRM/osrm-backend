#!/usr/bin/env bash

MASON_NAME=kdbush
MASON_VERSION=0.1.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mourner/kdbush.hpp/archive/v${MASON_VERSION}.tar.gz \
        e40170a84ee9fd6cff0a74577d70c22c570b0bdf

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/kdbush.hpp-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -v include/*.hpp ${MASON_PREFIX}/include
    cp -v README.md LICENSE ${MASON_PREFIX}
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    :
}


mason_run "$@"
