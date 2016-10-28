#!/usr/bin/env bash

MASON_NAME=luabind_luajit2.0.3
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
    ${MASON_DIR}/mason install luajit 2.0.3
    MASON_LUA=$(${MASON_DIR}/mason prefix luajit 2.0.3)
    ${MASON_DIR}/mason install boost 1.57.0
    MASON_BOOST_HEADERS=$(${MASON_DIR}/mason prefix boost 1.57.0)
    SYSTEM_ZLIB="/usr"
}

function mason_compile {
    mkdir build
    cd build
    cmake
    cmake ../ -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
      -DLUA_LIBRARIES=${MASON_LUA}/lib \
      -DLUA_INCLUDE_DIR=${MASON_LUA}/include/luajit-2.0 \
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
