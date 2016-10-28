#!/usr/bin/env bash

MASON_NAME=osmium-tool
MASON_VERSION=latest
MASON_LIB_FILE=bin/osmium

. ${MASON_DIR}/mason.sh

function mason_load_source {
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/osmium-tool-latest
    if [[ ! -d ${MASON_BUILD_PATH} ]]; then
        git clone --depth 1 https://github.com/osmcode/osmium-tool.git ${MASON_BUILD_PATH}
    else
        (cd ${MASON_BUILD_PATH} && git pull)
    fi
}

function mason_prepare_compile {
    echo ${MASON_ROOT}/.build
    cd ${MASON_ROOT}
    OSMIUM_INCLUDE_DIR=${MASON_ROOT}/osmcode-libosmium-latest/include
    curl --retry 3 -f -# -L "https://github.com/osmcode/libosmium/tarball/master" -o osmium.tar.gz
    rm -rf osmcode-libosmium-*
    tar -xzf osmium.tar.gz
    mv osmcode-libosmium-* osmcode-libosmium-latest

    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install boost 1.57.0
    ${MASON_DIR}/mason link boost 1.57.0
    ${MASON_DIR}/mason install boost_libprogram_options 1.57.0
    ${MASON_DIR}/mason link boost_libprogram_options 1.57.0
    ${MASON_DIR}/mason install protobuf 2.6.1
    ${MASON_DIR}/mason link protobuf 2.6.1
    ${MASON_DIR}/mason install zlib 1.2.8
    ${MASON_DIR}/mason link zlib 1.2.8
    ${MASON_DIR}/mason install expat 2.1.0
    ${MASON_DIR}/mason link expat 2.1.0
    ${MASON_DIR}/mason install osmpbf 1.3.3
    ${MASON_DIR}/mason link osmpbf 1.3.3
    ${MASON_DIR}/mason install bzip2 1.0.6
    ${MASON_DIR}/mason link bzip2 1.0.6
    ${MASON_DIR}/mason install geos 3.5.0
    ${MASON_DIR}/mason link geos 3.5.0
}

function mason_compile {
    mkdir build
    cd build
    CMAKE_PREFIX_PATH=${MASON_ROOT}/.link \
    cmake \
        -DOSMIUM_INCLUDE_DIR=${OSMIUM_INCLUDE_DIR} \
        -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
        ..
    make install
}

function mason_clean {
    make clean
}

mason_run "$@"
