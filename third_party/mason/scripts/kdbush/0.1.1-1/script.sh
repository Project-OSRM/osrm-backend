#!/usr/bin/env bash

MASON_NAME=kdbush
MASON_VERSION=0.1.1-1
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mourner/kdbush.hpp/archive/v0.1.1.tar.gz \
        a7ce2860374fc547fbf4568c6c77eba6af376e5a

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/kdbush.hpp-0.1.1
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
