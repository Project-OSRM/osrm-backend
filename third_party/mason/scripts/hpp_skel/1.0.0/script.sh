#!/usr/bin/env bash

MASON_NAME=hpp_skel
MASON_VERSION=1.0.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/hpp-skel/archive/v${MASON_VERSION}.tar.gz \
        0a869f57141a52e3d2f18fb8f857bd93c3f4f044

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/hpp-skel-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -r include/hello_world ${MASON_PREFIX}/include/hello_world
}

mason_run "$@"