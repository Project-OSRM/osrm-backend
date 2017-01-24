#!/usr/bin/env bash

MASON_NAME=tippecanoe
MASON_VERSION=1.9.7
MASON_LIB_FILE=bin/tippecanoe

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/tippecanoe/tarball/1.9.7 \
        efc54e601e93f3b965c6d350f2fa73c81384b720

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/mapbox-tippecanoe-8cc844c
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install protobuf 2.6.1
    ${MASON_DIR}/mason link protobuf 2.6.1
    ${MASON_DIR}/mason install sqlite 3.9.1
    ${MASON_DIR}/mason link sqlite 3.9.1
}

function mason_compile {
    PREFIX=${MASON_PREFIX} \
    PATH=${MASON_DIR}/mason_packages/.link/bin:${PATH} \
    CXXFLAGS=-I${MASON_DIR}/mason_packages/.link/include \
    LDFLAGS="-L${MASON_DIR}/mason_packages/.link/lib -lprotobuf-lite -ldl -lpthread" make
    
    PREFIX=${MASON_PREFIX} \
    PATH=${MASON_DIR}/mason_packages/.link/bin:${PATH} \
    CXXFLAGS=-I${MASON_DIR}/mason_packages/.link/include \
    LDFLAGS="-L${MASON_DIR}/mason_packages/.link/lib -lprotobuf-lite -ldl -lpthread" make install
}

function mason_clean {
    make clean
}

mason_run "$@"
