#!/usr/bin/env bash

MASON_NAME=protobuf
MASON_VERSION=3.2.0
MASON_LIB_FILE=lib/libprotobuf-lite.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/protobuf-lite.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/protobuf/releases/download/v${MASON_VERSION}/protobuf-cpp-${MASON_VERSION}.tar.gz \
        29792bf4d86237e41118d692dc11ca03dbe796d8

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    # note CFLAGS overrides defaults (-O2 -g -DNDEBUG) so we need to add optimization flags back
    export CFLAGS="${CFLAGS} -O3 -DNDEBUG"
    export CXXFLAGS="${CXXFLAGS} -O3 -DNDEBUG"
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static --disable-shared \
        --disable-debug --without-zlib \
        --disable-dependency-tracking

    make V=1 -j${MASON_CONCURRENCY}
    make install -j${MASON_CONCURRENCY}
}

function mason_clean {
    make clean
}

mason_run "$@"
