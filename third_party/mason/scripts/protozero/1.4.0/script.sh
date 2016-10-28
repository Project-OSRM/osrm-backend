#!/usr/bin/env bash

MASON_NAME=protozero
MASON_VERSION=1.4.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/protozero/tarball/v1.4.0 \
        0f6eb6234ee5a258216b2f9a83e35a1f5e60ca03

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/mapbox-protozero-ca34d86
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -r include/protozero ${MASON_PREFIX}/include/protozero
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    :
}

mason_run "$@"
