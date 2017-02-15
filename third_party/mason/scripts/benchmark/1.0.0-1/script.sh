#!/usr/bin/env bash

MASON_NAME=benchmark
MASON_VERSION=1.0.0-1
MASON_LIB_FILE=lib/libbenchmark.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/benchmark/archive/v1.0.0.tar.gz \
        dcf87e5faead951fd1e9ab103cb36a7c8ebe4837

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/benchmark-1.0.0
}

function mason_compile {
    rm -rf build
    mkdir -p build
    cd build
    if [ ${MASON_PLATFORM} == 'ios' ] ; then
        # Make sure CMake thinks we're cross-compiling and manually set the exit codes
        # because CMake can't run the test programs
        echo "set (CMAKE_SYSTEM_NAME Darwin)" > toolchain.cmake
        cmake \
            -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
            -DRUN_HAVE_STD_REGEX=1 \
            -DRUN_HAVE_POSIX_REGEX=0 \
            -DRUN_HAVE_STEADY_CLOCK=0 \
            -DCMAKE_CXX_FLAGS="${CFLAGS:-}" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="${MASON_PREFIX}" \
            -DBENCHMARK_ENABLE_LTO=ON \
            -DBENCHMARK_ENABLE_TESTING=OFF \
            ..
    else
        cmake \
            ${MASON_CMAKE_TOOLCHAIN} \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="${MASON_PREFIX}" \
            -DBENCHMARK_ENABLE_LTO=ON \
            -DBENCHMARK_ENABLE_TESTING=OFF \
            ..
    fi

    make install -j${MASON_CONCURRENCY}
}

function mason_cflags {
    echo -isystem ${MASON_PREFIX}/include
}

function mason_ldflags {
    echo -lpthread
}

function mason_static_libs {
    echo ${MASON_PREFIX}/${MASON_LIB_FILE}
}

mason_run "$@"
