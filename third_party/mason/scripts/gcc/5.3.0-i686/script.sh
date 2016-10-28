#!/usr/bin/env bash

MASON_NAME=gcc
MASON_VERSION=5.3.0-i686
MASON_LIB_FILE=root/bin/arm-i686-linux-gnueabi-gcc

. ${MASON_DIR}/mason.sh

if [ -f ${MASON_PREFIX}/setup.sh ] ; then
    . ${MASON_PREFIX}/setup.sh
fi

function mason_try_binary {
    MASON_XC_ORIGIN="${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}"
    . ${MASON_XC_ORIGIN}/setup.sh

    mkdir -p ${MASON_PREFIX}
    cp "${MASON_XC_ORIGIN}/toolchain.cmake" ${MASON_PREFIX}/
    cp "${MASON_XC_ORIGIN}/toolchain.sh" ${MASON_PREFIX}/
    mason_success "Installed toolchain setup files at ${MASON_PREFIX}"
}

function mason_load_source {
    :
}

function mason_compile {
    :
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

mason_run "$@"
