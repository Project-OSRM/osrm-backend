#!/usr/bin/env bash

MASON_NAME=lua
MASON_VERSION=5.2.4
MASON_LIB_FILE=lib/liblua.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.lua.org/ftp/lua-${MASON_VERSION}.tar.gz \
        6dd4526fdae5a7f76e44febf4d3066614920c43e

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    mason_step "Loading patch ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff"
    patch -N -p1 < ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff
    make generic CC=$CC MYCFLAGS="${CFLAGS}" MYLDFLAGS="${LDFLAGS}" INSTALL_TOP=${MASON_PREFIX} install
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -llua"
}

function mason_clean {
    make clean
}

mason_run "$@"
