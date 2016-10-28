#!/usr/bin/env bash

MASON_NAME=supercluster
MASON_VERSION=0.1.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/supercluster.hpp/archive/v${MASON_VERSION}.tar.gz \
        323f09e49b327312b3583fc1ae5ec7c48964aa1a

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/supercluster.hpp-${MASON_VERSION}
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
