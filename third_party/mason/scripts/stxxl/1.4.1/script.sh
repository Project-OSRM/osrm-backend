#!/usr/bin/env bash

MASON_NAME=stxxl
MASON_VERSION=1.4.1
MASON_LIB_FILE=lib/libstxxl.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/stxxl/stxxl/releases/download/1.4.1/stxxl-1.4.1.tar.gz \
        68c0d402ec19e0d254b5b2b217a8c9c5d759a9ef

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    mkdir build
    cd build
    cmake 
    cmake ../ -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
     -DBUILD_STATIC_LIBS=ON \
     -DUSE_GNU_PARALLEL=OFF \
     -DCMAKE_BUILD_TYPE=Release
    make -j${MASON_CONCURRENCY} VERBOSE=1
    make install
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -lstxxl"
}

function mason_clean {
    make clean
}

mason_run "$@"
