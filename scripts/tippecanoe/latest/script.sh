#!/usr/bin/env bash

MASON_NAME=tippecanoe
MASON_VERSION=latest
MASON_LIB_FILE=bin/tippecanoe

. ${MASON_DIR}/mason.sh

function mason_load_source {
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/tippecanoe-latest
    if [[ ! -d ${MASON_BUILD_PATH} ]]; then
        git clone --depth 1 https://github.com/mapbox/tippecanoe.git ${MASON_BUILD_PATH}
    else
        (cd ${MASON_BUILD_PATH} && git pull)
    fi
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
