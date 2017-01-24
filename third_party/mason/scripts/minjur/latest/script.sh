#!/usr/bin/env bash

MASON_NAME=minjur
MASON_VERSION=latest
MASON_LIB_FILE=bin/minjur

. ${MASON_DIR}/mason.sh

function mason_load_source {
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/minjur-latest
    if [[ ! -d ${MASON_BUILD_PATH} ]]; then
        git clone --depth 1 https://github.com/mapbox/minjur.git ${MASON_BUILD_PATH}
    else
        (cd ${MASON_BUILD_PATH} && git pull)
    fi
}

function mason_prepare_compile {
    ${MASON_DIR}/mason install cmake 3.6.2
    ${MASON_DIR}/mason link cmake 3.6.2
    ${MASON_DIR}/mason install utfcpp 2.3.4
    ${MASON_DIR}/mason link utfcpp 2.3.4
    ${MASON_DIR}/mason install protozero 1.4.5
    ${MASON_DIR}/mason link protozero 1.4.5
    ${MASON_DIR}/mason install rapidjson 2016-07-20-369de87
    ${MASON_DIR}/mason link rapidjson 2016-07-20-369de87
    ${MASON_DIR}/mason install libosmium 2.10.3
    ${MASON_DIR}/mason link libosmium 2.10.3
    ${MASON_DIR}/mason install boost 1.62.0
    ${MASON_DIR}/mason link boost 1.62.0
    ${MASON_DIR}/mason install zlib 1.2.8
    ${MASON_DIR}/mason link zlib 1.2.8
    ${MASON_DIR}/mason install expat 2.2.0
    ${MASON_DIR}/mason link expat 2.2.0
    ${MASON_DIR}/mason install bzip2 1.0.6
    ${MASON_DIR}/mason link bzip2 1.0.6
}

function mason_compile {
    rm -rf build
    mkdir -p build
    cd build
    CMAKE_PREFIX_PATH=${MASON_ROOT}/.link \
    ${MASON_ROOT}/.link/bin/cmake \
        -DCMAKE_BUILD_TYPE=Release \
        ..
    make VERBOSE=1
    mkdir -p ${MASON_PREFIX}/bin
    mv minjur ${MASON_PREFIX}/bin/minjur
    mv minjur-mp ${MASON_PREFIX}/bin/minjur-mp
    mv minjur-generate-tilelist ${MASON_PREFIX}/bin/minjur-generate-tilelist
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
