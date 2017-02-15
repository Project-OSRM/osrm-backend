#!/usr/bin/env bash

MASON_NAME=libgdal
MASON_VERSION=2.1.3
MASON_LIB_FILE=lib/libgdal.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
    mkdir -p ${MASON_BUILD_PATH}
}

function mason_prepare_compile {
    ${MASON_DIR}/mason install gdal ${MASON_VERSION}
    GDAL_PREFIX=$(${MASON_DIR}/mason prefix gdal ${MASON_VERSION})
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/lib
    mkdir -p ${MASON_PREFIX}/include
    mkdir -p ${MASON_PREFIX}/share
    mkdir -p ${MASON_PREFIX}/bin
    cp -r ${GDAL_PREFIX}/bin/gdal-config ${MASON_PREFIX}/bin/
    cp -r ${GDAL_PREFIX}/include/* ${MASON_PREFIX}/include/
    cp -r ${GDAL_PREFIX}/share/* ${MASON_PREFIX}/share/
    cp -r ${GDAL_PREFIX}/lib/libgdal.a ${MASON_PREFIX}/lib/
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo $(${MASON_PREFIX}/bin/gdal-config --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"
