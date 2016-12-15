#!/usr/bin/env bash

MASON_NAME=luabind_lua524
MASON_VERSION=e414c57bcb687bb3091b7c55bbff6947f052e46b
MASON_LIB_FILE=lib/libluabind.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/luabind/archive/${MASON_VERSION}.tar.gz \
        49bbe06214a6a747a1f20eba34b7c2d0ea41c51a

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/luabind-${MASON_VERSION}
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install lua 5.2.4
    MASON_LUA=$(${MASON_DIR}/mason prefix lua 5.2.4)
    ${MASON_DIR}/mason install boost 1.61.0
    MASON_BOOST_HEADERS=$(${MASON_DIR}/mason prefix boost 1.61.0)
    ${MASON_DIR}/mason install cmake 3.5.2
    MASON_CMAKE=$(${MASON_DIR}/mason prefix cmake 3.5.2)
}

function mason_compile {
    rm -rf build
    mkdir build
    cd build
    ${MASON_CMAKE}/bin/cmake ../ -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
      -DCMAKE_CXX_FLAGS="${CXXFLAGS} -DLUA_COMPAT_ALL" \
      -DLUA_LIBRARIES=${MASON_LUA}/lib \
      -DLUA_INCLUDE_DIR=${MASON_LUA}/include \
      -DBOOST_INCLUDEDIR=${MASON_BOOST_HEADERS}/include \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF
    make -j${MASON_CONCURRENCY} VERBOSE=1
    make install
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -lluabind"
}

function mason_clean {
    make clean
}

mason_run "$@"
