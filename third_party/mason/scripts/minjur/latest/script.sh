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
    echo ${MASON_ROOT}/.build
    cd ${MASON_ROOT}
    OSMIUM_INCLUDE_DIR=${MASON_ROOT}/osmcode-libosmium-latest/include
    curl --retry 3 -f -# -L "https://github.com/osmcode/libosmium/tarball/master" -o osmium.tar.gz
    tar -xzf osmium.tar.gz
    rm -rf osmcode-libosmium-latest
    mv osmcode-libosmium-* osmcode-libosmium-latest

    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install cmake 3.2.2
    ${MASON_DIR}/mason link cmake 3.2.2
    ${MASON_DIR}/mason install boost 1.59.0
    ${MASON_DIR}/mason link boost 1.59.0
    ${MASON_DIR}/mason install boost_liball 1.59.0
    ${MASON_DIR}/mason link boost_liball 1.59.0
    ${MASON_DIR}/mason install zlib 1.2.8
    ${MASON_DIR}/mason link zlib 1.2.8
    ${MASON_DIR}/mason install expat 2.1.0
    ${MASON_DIR}/mason link expat 2.1.0
    ${MASON_DIR}/mason install bzip 1.0.6
    ${MASON_DIR}/mason link bzip 1.0.6
}

function mason_compile {
    rm -rf build
    mkdir -p build
    cd build
    CMAKE_PREFIX_PATH=${MASON_ROOT}/.link \
    ${MASON_ROOT}/.link/bin/cmake \
        -DOSMIUM_INCLUDE_DIR=${OSMIUM_INCLUDE_DIR} \
        ..
    make
    mkdir -p ${MASON_PREFIX}/bin
    mv minjur ${MASON_PREFIX}/bin/minjur
    mv minjur-mp ${MASON_PREFIX}/bin/minjur-mp
    mv minjur-generate-tilelist ${MASON_PREFIX}/bin/minjur-generate-tilelist
}

function mason_clean {
    make clean
}

mason_run "$@"
