#!/usr/bin/env bash

MASON_NAME=tippecanoe
MASON_VERSION=1.15.1
MASON_LIB_FILE=bin/tippecanoe

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/tippecanoe/archive/${MASON_VERSION}.tar.gz \
        0b31a40e7ababf34fa5945afc538224d86f6b729

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    ${MASON_DIR}/mason install sqlite 3.14.2
    MASON_SQLITE=$(${MASON_DIR}/mason prefix sqlite 3.14.2)
}

function mason_compile {
    PREFIX=${MASON_PREFIX} \
    PATH=${MASON_SQLITE}/bin:${PATH} \
    CXXFLAGS="${CXXFLAGS} -I${MASON_SQLITE}/include" \
    LDFLAGS="${LDFLAGS} -L${MASON_SQLITE}/lib -ldl -lpthread" make

    PREFIX=${MASON_PREFIX} \
    PATH=${MASON_SQLITE}/bin:${PATH} \
    CXXFLAGS="${CXXFLAGS} -I${MASON_SQLITE}/include" \
    LDFLAGS="${LDFLAGS} -L${MASON_SQLITE}/lib -ldl -lpthread" make install
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
