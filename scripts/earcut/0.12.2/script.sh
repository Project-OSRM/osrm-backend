#!/usr/bin/env bash

MASON_NAME=earcut
MASON_VERSION=0.12.2
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/earcut.hpp/archive/v${MASON_VERSION}.tar.gz \
        fcc64054afaf4f908b72e51944796339064344f5
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/earcut.hpp-${MASON_VERSION}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/mapbox
    cp -v include/mapbox/earcut.hpp ${MASON_PREFIX}/include/mapbox/earcut.hpp
    cp -v README.md LICENSE ${MASON_PREFIX}
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

mason_run "$@"
