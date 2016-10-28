#!/usr/bin/env bash

MASON_NAME=osmium-tool
MASON_VERSION=1.3.0
MASON_LIB_FILE=bin/osmium

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/osmcode/osmium-tool/tarball/v1.3.0 \
        b7be94a999061bb0dbfb04451cb2b2a8a09c1d23

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/osmcode-osmium-tool-24de9be
}

function mason_prepare_compile {
    echo ${MASON_ROOT}/.build
    cd ${MASON_ROOT}
    OSMIUM_INCLUDE_DIR=${MASON_ROOT}/osmcode-libosmium-0ff2780/include
    curl --retry 3 -f -# -L "https://github.com/osmcode/libosmium/tarball/v2.5.4" -o osmium.tar.gz
    tar -xzf osmium.tar.gz

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
    ${MASON_DIR}/mason install bzip 1.0.6
    ${MASON_DIR}/mason link bzip 1.0.6
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
