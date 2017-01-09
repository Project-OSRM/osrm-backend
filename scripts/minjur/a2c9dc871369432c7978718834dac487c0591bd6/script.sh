#!/usr/bin/env bash

MASON_NAME=minjur
MASON_VERSION=a2c9dc871369432c7978718834dac487c0591bd6

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/minjur/tarball/a2c9dc871369432c7978718834dac487c0591bd6 \
        b24a45f64ae0b75e2fbcbb6b87e04192b63b3014

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/mapbox-minjur-a2c9dc8
}

function mason_prepare_compile {
    echo ${MASON_ROOT}/.build
    cd ${MASON_ROOT}
    OSMIUM_INCLUDE_DIR=${MASON_ROOT}/osmcode-libosmium-372d29a/include
    curl --retry 3 -f -# -L "https://github.com/osmcode/libosmium/tarball/372d29a34d8b3f571ea7172d527730d3d5200dab" -o osmium.tar.gz
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
        -DCMAKE_BUILD_TYPE=Release \
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
