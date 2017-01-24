#!/usr/bin/env bash

MASON_NAME=libuv
MASON_VERSION=0.10.33
MASON_LIB_FILE=lib/libuv.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/libuv/libuv/archive/v${MASON_VERSION}.tar.gz \
        d4bc9973e0cc4b653ebc4fccaccb9564c022b52b

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libuv-${MASON_VERSION}
}

function mason_compile {
    CFLAGS="-fPIC ${CFLAGS:-}" make libuv.a -j${MASON_CONCURRENCY}
    mkdir -p lib
    mv libuv.a "${MASON_LIB_FILE}"
    mkdir -p "${MASON_PREFIX}"
    cp -rv lib include "${MASON_PREFIX}"
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        echo "-lpthread -ldl -framework CoreFoundation -framework CoreServices"
    elif [ ${MASON_PLATFORM} = 'ios' ]; then
        echo "-lpthread -ldl"
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        echo "-lpthread -ldl -lrt"
    fi
}

function mason_clean {
    make clean
}

mason_run "$@"
