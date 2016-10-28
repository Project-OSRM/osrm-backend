#!/usr/bin/env bash

MASON_NAME=hpp_skel
MASON_VERSION=0.0.1
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/hpp-skel/archive/v${MASON_VERSION}.tar.gz \
        2876991412fcfd41bd7d606e78025e2d3f6e319b

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/hpp-skel-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -r include ${MASON_PREFIX}/include
}

mason_run "$@"