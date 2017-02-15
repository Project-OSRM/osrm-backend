#!/usr/bin/env bash

MASON_NAME=vector-tile
MASON_VERSION=1.0.0-rc4
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/${MASON_NAME}/archive/v${MASON_VERSION}.tar.gz \
        7f9e12913ccb09bbe4e32cc8300383b1e7a2aef0

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -r include/mapbox ${MASON_PREFIX}/include/mapbox
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

mason_run "$@"