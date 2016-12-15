#!/usr/bin/env bash

MASON_NAME=7z
MASON_VERSION=9.20.1
MASON_LIB_FILE=bin/7z

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://downloads.sourceforge.net/project/p7zip/p7zip/9.20.1/p7zip_9.20.1_src_all.tar.bz2 \
        30b1ff90105134947c67427bfc5c570857051f50

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/p7zip_${MASON_VERSION}
}

function mason_prepare_compile {
    if [ $(uname -s) = 'Darwin' ]; then
        cp makefile.macosx_64bits makefile.machine
    elif [ $(uname -s) = 'Linux' ]; then
        cp makefile.linux_clang_amd64 makefile.linux
    fi
}

function mason_compile {
    make all3 -j${MASON_CONCURRENCY}
    mkdir -p "${MASON_PREFIX}"
    cp -rv bin "${MASON_PREFIX}"
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
