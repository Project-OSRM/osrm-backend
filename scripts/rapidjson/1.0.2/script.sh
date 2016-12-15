#!/usr/bin/env bash

MASON_NAME=rapidjson
MASON_VERSION=1.0.2
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
    https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz \
    dada19604224e5040914181b67b536e9cf24dcbe
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/rapidjson-1.0.2
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    cp -rv include ${MASON_PREFIX}
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    : # We're only using the full path to the archive, which is output in static_libs
}

function mason_clean {
    make clean
}

mason_run "$@"
