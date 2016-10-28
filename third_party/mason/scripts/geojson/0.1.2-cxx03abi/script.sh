#!/usr/bin/env bash

LIB_VERSION=0.1.2

MASON_NAME=geojson
MASON_VERSION=${LIB_VERSION}-cxx03abi
MASON_LIB_FILE=lib/libgeojson.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/geojson-cpp/archive/v${LIB_VERSION}.tar.gz \
        ba0f620b97c8ab39b262f8474038a3fd4e33a7f3

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/geojson-cpp-${LIB_VERSION}
}

function mason_compile {
    make clean
    MASON=${MASON_DIR}/mason make
    mkdir -p ${MASON_PREFIX}/{include,lib}
    cp -r include/mapbox ${MASON_PREFIX}/include/mapbox
    mv build/libgeojson.a ${MASON_PREFIX}/lib
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    :
}

function mason_static_libs {
    echo ${MASON_PREFIX}/lib/libgeojson.a
}

mason_run "$@"
