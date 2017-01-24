#!/usr/bin/env bash

MASON_NAME=utfcpp
MASON_VERSION=2.3.4
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/nemtrif/utfcpp/tarball/v2.3.4 \
        8df46127dfc5371efe7435fba947946e4081dc58

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/nemtrif-utfcpp-be66c3f
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -r source/utf8 ${MASON_PREFIX}/include/utf8
    cp source/utf8.h ${MASON_PREFIX}/include/
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
