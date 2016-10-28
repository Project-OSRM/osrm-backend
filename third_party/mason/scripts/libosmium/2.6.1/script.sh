#!/usr/bin/env bash

MASON_NAME=libosmium
MASON_VERSION=2.6.1
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/osmcode/libosmium/tarball/v2.6.1 \
        079814b4632dcc585cef6e6b0f1e1e1d27b53904

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/osmcode-libosmium-2282c84
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -r include/osmium ${MASON_PREFIX}/include/osmium
}

mason_run "$@"
