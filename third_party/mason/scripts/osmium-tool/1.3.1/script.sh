#!/usr/bin/env bash

MASON_NAME=osmium-tool
MASON_VERSION=1.3.1
MASON_LIB_FILE=bin/osmium

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/osmcode/osmium-tool/tarball/v1.3.1 \
        1718acc22f3d92d74967653d2194386d61746db3

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/osmcode-osmium-tool-52ee5b7
}

function mason_prepare_compile {
    BOOST_VERSION=1.61.0
    ${MASON_DIR}/mason install cmake 3.5.2
    MASON_CMAKE=$(${MASON_DIR}/mason prefix cmake 3.5.2)
    ${MASON_DIR}/mason install boost ${BOOST_VERSION}
    ${MASON_DIR}/mason link boost ${BOOST_VERSION}
    ${MASON_DIR}/mason install boost_libprogram_options ${BOOST_VERSION}
    ${MASON_DIR}/mason link boost_libprogram_options ${BOOST_VERSION}
    ${MASON_DIR}/mason install libosmium 2.8.0
    ${MASON_DIR}/mason link libosmium 2.8.0
    ${MASON_DIR}/mason install protozero 1.4.0
    ${MASON_DIR}/mason link protozero 1.4.0
    ${MASON_DIR}/mason install utfcpp 2.3.4
    ${MASON_DIR}/mason link utfcpp 2.3.4
    ${MASON_DIR}/mason install zlib 1.2.8
    ${MASON_DIR}/mason link zlib 1.2.8
    ${MASON_DIR}/mason install expat 2.1.0
    ${MASON_DIR}/mason link expat 2.1.0
    ${MASON_DIR}/mason install bzip2 1.0.6
    ${MASON_DIR}/mason link bzip2 1.0.6
    MASON_HOME=${MASON_ROOT}/.link/

}

function mason_compile {
    rm -rf ./build/
    mkdir ./build
    cd ./build
    echo $MASON_HOME
    ${MASON_CMAKE}/bin/cmake ../ \
        -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
        -DCMAKE_BUILD_TYPE=Release \
        -DBoost_NO_SYSTEM_PATHS=ON \
        -DBoost_USE_STATIC_LIBS=ON \
        -DOSMIUM_INCLUDE_DIR=${MASON_HOME}/include \
        -DCMAKE_INCLUDE_PATH=${MASON_HOME}/include \
        -DCMAKE_LIBRARY_PATH=${MASON_HOME}/lib
    make install
}

function mason_clean {
    make clean
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

mason_run "$@"
