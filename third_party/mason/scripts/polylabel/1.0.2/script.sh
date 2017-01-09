#!/usr/bin/env bash

MASON_NAME=polylabel
MASON_VERSION=1.0.2
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
    https://github.com/mapbox/polylabel/archive/ad5b37a15502507198f62d1ced608cebf0abf6cf.tar.gz \
    7f982a293c5024ad170c8b54977189a995486615
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-ad5b37a15502507198f62d1ced608cebf0abf6cf
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/mapbox
    cp -v polylabel.hpp ${MASON_PREFIX}/include/mapbox/polylabel.hpp
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
