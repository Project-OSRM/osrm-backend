#!/usr/bin/env bash

MASON_NAME=luajit
MASON_VERSION=2.0.3
MASON_LIB_FILE=lib/libluajit-5.1.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://luajit.org/download/LuaJIT-2.0.3.tar.gz \
        0705f5967736b3b01fe7b96af03a8fff45fa43d8

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/LuaJIT-${MASON_VERSION}
}

function mason_compile {
    make CC=$CC CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" PREFIX=${MASON_PREFIX} install
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "ld ${MASON_PREFIX}/lib/libluajit-5.1.a"
}

function mason_clean {
    make clean
}

mason_run "$@"
