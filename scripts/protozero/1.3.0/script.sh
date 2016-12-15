#!/usr/bin/env bash

MASON_NAME=protozero
MASON_VERSION=1.3.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/protozero/tarball/v1.3.0 \
        6d0432ccb300fda98805ad429a1fac980d42510e

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/mapbox-protozero-ad3ca5b
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
