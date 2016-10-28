#!/usr/bin/env bash

MASON_NAME=earcut
MASON_VERSION=0.9-pool
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
    https://github.com/mapbox/earcut.hpp/archive/v0.9-pool.tar.gz \
    b3dacbd180a5480d797b32ec3bc242b688046e99
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/earcut.hpp-0.9-pool
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
