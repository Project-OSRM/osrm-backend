#!/usr/bin/env bash

LIB_VERSION=5.0.0

MASON_NAME=geojsonvt
MASON_VERSION=${LIB_VERSION}
MASON_LIB_FILE=lib/libgeojsonvt.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/geojson-vt-cpp/archive/v${LIB_VERSION}.tar.gz \
        9ab6c25076fc042f04f5488ba3726579a4b55d77

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/geojson-vt-cpp-${LIB_VERSION}
}

function mason_compile {
    # setup mason
    rm -rf .mason
    ln -s ${MASON_DIR} .mason

    # build
    INSTALL_PREFIX=${MASON_PREFIX} ./configure
    CXXFLAGS="-fPIC ${CFLAGS:-} ${CXXFLAGS:-}" make install
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    :
}

function mason_static_libs {
    echo ${MASON_PREFIX}/lib/libgeojsonvt.a
}

mason_run "$@"
