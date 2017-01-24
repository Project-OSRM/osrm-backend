#!/usr/bin/env bash

MASON_NAME=js-number-to-string
MASON_VERSION=1.0.3
MASON_LIB_FILE=lib/libjs-number-to-string.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/js-number-to-string/archive/v${MASON_VERSION}.tar.gz \
        b85f911c51309c77c5bd3239c74a62129bd6bc1c

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/js-number-to-string-${MASON_VERSION}
}

function mason_compile {
    rm -rf build
    mkdir build
    cd build
    CMAKE_TOOLCHAIN_FILE=
    if [ ${MASON_PLATFORM} == 'ios' ] ; then
        # Make sure CMake thinks we're cross-compiling and manually set the exit codes
        # because CMake can't run the test programs
        echo "set (CMAKE_SYSTEM_NAME Darwin)" > toolchain.cmake
        CMAKE_TOOLCHAIN_FILE="-DCMAKE_TOOLCHAIN_FILE=toolchain.cmake"
    elif [ ${MASON_PLATFORM} == 'android' ] ; then
        # Make sure CMake thinks we're cross-compiling and manually set the exit codes
        # because CMake can't run the test programs
        echo "set (CMAKE_SYSTEM_NAME Android)" > toolchain.cmake
        CMAKE_TOOLCHAIN_FILE="-DCMAKE_TOOLCHAIN_FILE=toolchain.cmake"
    fi

    cmake .. ${CMAKE_TOOLCHAIN_FILE} \
      -DCMAKE_CXX_FLAGS="${CFLAGS:-}" \
      -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=OFF

    make -j${MASON_CONCURRENCY} VERBOSE=1
    make install
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -ljs-number-to-string"
}

function mason_clean {
    make clean
}

mason_run "$@"
