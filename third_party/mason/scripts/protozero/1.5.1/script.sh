#!/usr/bin/env bash

MASON_NAME=protozero
MASON_VERSION=1.5.1
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/protozero/archive/v${MASON_VERSION}.tar.gz \
        d36e3a99b7e29383455459df723dbbc0a83a85ae

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/protozero-${MASON_VERSION}
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
