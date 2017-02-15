#!/usr/bin/env bash

MASON_NAME=gflags
MASON_VERSION=2.1.2
MASON_LIB_FILE=lib/libgflags.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/gflags/${MASON_NAME}/archive/v${MASON_VERSION}.tar.gz \
        6810db9e9cb378bfc0b0fb250f27f4416df5beec

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    CCACHE_VERSION=3.3.1
    ${MASON_DIR}/mason install ccache ${CCACHE_VERSION}
    MASON_CCACHE=$(${MASON_DIR}/mason prefix ccache ${CCACHE_VERSION})
    ${MASON_DIR}/mason install cmake 3.7.2
    ${MASON_DIR}/mason link cmake 3.7.2
}

function mason_compile {
    rm -rf build
    mkdir -p build
    cd build
    CMAKE_PREFIX_PATH=${MASON_ROOT}/.link \
    ${MASON_ROOT}/.link/bin/cmake \
        -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
        -DCMAKE_CXX_COMPILER_LAUNCHER="${MASON_CCACHE}/bin/ccache" \
        -DCMAKE_BUILD_TYPE=Release \
        ..
    make VERBOSE=1 -j${MASON_CONCURRENCY}
    make install

}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_static_libs {
    echo ${MASON_PREFIX}/${MASON_LIB_FILE}
}

mason_run "$@"
