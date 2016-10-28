#!/usr/bin/env bash

LIB_VERSION=1.7.0

MASON_NAME=gtest
MASON_VERSION=${LIB_VERSION}-cxx11abi
MASON_LIB_FILE=lib/libgtest.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://googletest.googlecode.com/files/gtest-${LIB_VERSION}.zip \
        86f5fd0ce20ef1283092d1d5a4bc004916521aaf


    mason_setup_build_dir
    unzip ../.cache/${MASON_SLUG}

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/gtest-${LIB_VERSION}
}

function mason_compile {
    if [ ${MASON_PLATFORM} = 'ios' ]; then
        ${CXX:-c++} ${CFLAGS:-} -O3 -g -isystem fused-src -pthread -c -fPIC fused-src/gtest/gtest-all.cc
        mkdir -p lib/.libs
        libtool -static gtest-all.o -o lib/.libs/libgtest.a
    else
        ./configure \
            --prefix=${MASON_PREFIX} \
            ${MASON_HOST_ARG} \
            --enable-static \
            --with-pic \
            --disable-shared \
            --disable-dependency-tracking

        make -j${MASON_CONCURRENCY}
    fi

    mkdir -p ${MASON_PREFIX}/lib
    cp -v lib/.libs/libgtest.a ${MASON_PREFIX}/lib
    mkdir -p ${MASON_PREFIX}/include/gtest
    cp -v fused-src/gtest/gtest.h ${MASON_PREFIX}/include/gtest
}

function mason_cflags {
    echo -isystem ${MASON_PREFIX}/include -I${MASON_PREFIX}/include
}

function mason_ldflags {
    echo -lpthread
}

function mason_static_libs {
    echo ${MASON_PREFIX}/lib/libgtest.a
}


mason_run "$@"
