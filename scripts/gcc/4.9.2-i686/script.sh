#!/usr/bin/env bash

MASON_NAME=gcc
MASON_VERSION=4.9.2-i686
MASON_LIB_FILE=root/bin/i686-pc-linux-gnu-gcc

. ${MASON_DIR}/mason.sh

if [ -f ${MASON_PREFIX}/setup.sh ]; then
    . ${MASON_PREFIX}/setup.sh
fi

function mason_load_source {
    mkdir -p ${MASON_ROOT}
    echo ${MASON_ROOT}
    export MASON_BUILD_PATH=${MASON_ROOT}/../toolchain/${MASON_PLATFORM}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    cp -av ${MASON_BUILD_PATH}/* ${MASON_PREFIX}/
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

mason_run "$@"
