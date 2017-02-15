#!/usr/bin/env bash

MASON_NAME=jni.hpp
MASON_VERSION=3.0.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
    https://github.com/mapbox/jni.hpp/archive/v${MASON_VERSION}.tar.gz \
    abc0e127abfe0ce7992e29f0f1b51877495ad4f0
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/jni.hpp-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    cp -vR include README.md LICENSE.txt ${MASON_PREFIX}
}

function mason_cflags {
    echo -isystem ${MASON_PREFIX}/include -I${MASON_PREFIX}/include
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

mason_run "$@"