#!/usr/bin/env bash

MASON_NAME=earcut
MASON_VERSION=0.10
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
    https://github.com/mapbox/earcut.hpp/archive/v${MASON_VERSION}.tar.gz \
    a66ce3dc19dbc059d42da52dacb0dd380286555d
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/earcut.hpp-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/mapbox
    cp -v include/*.hpp ${MASON_PREFIX}/include/mapbox
    cp -v README.md LICENSE ${MASON_PREFIX}
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
