#!/usr/bin/env bash

MASON_NAME=osmpbf
MASON_VERSION=1.3.3
MASON_LIB_FILE=lib/libosmpbf.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/scrosby/OSM-binary/tarball/v1.3.3 \
        ae165e355532e37f1ca60a36395abef959a12a81

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/scrosby-OSM-binary-72cd8e5
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install protobuf 2.6.1
    ${MASON_DIR}/mason link protobuf 2.6.1
}

function mason_compile {
    PROTOC="${MASON_ROOT}/.link/bin/protoc" \
    LDFLAGS="-L${MASON_ROOT}/.link/lib" \
    CXXFLAGS="-I${MASON_ROOT}/.link/include" \
    make -C src

    PREFIX="${MASON_PREFIX}" make -C src install
}

function mason_clean {
    make clean
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_lflags {
    echo "-L${MASON_PREFIX}/lib"
}


mason_run "$@"
