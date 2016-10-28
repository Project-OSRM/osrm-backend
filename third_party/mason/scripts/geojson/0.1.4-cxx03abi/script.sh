#!/usr/bin/env bash

LIB_VERSION=0.1.4
CXXABI=-D_GLIBCXX_USE_CXX11_ABI=0

MASON_NAME=geojson
MASON_VERSION=${LIB_VERSION}-cxx03abi
MASON_LIB_FILE=lib/libgeojson.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/geojson-cpp/archive/v${LIB_VERSION}.tar.gz \
        f0a1499e67a9641691933eaec06cc74c7b70f468

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/geojson-cpp-${LIB_VERSION}
}

function mason_compile {
    make clean
    CXXFLAGS="$CXXFLAGS $CXXABI" MASON=${MASON_DIR}/mason make
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
