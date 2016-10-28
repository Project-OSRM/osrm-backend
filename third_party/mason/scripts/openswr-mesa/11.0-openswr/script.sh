#!/usr/bin/env bash

MASON_NAME=openswr-mesa
MASON_VERSION=11.0-openswr
MASON_LIB_FILE=lib/libGL.so
MASON_PKGCONFIG_FILE=lib/pkgconfig/gl.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/OpenSWR/openswr-mesa/archive/11.0-openswr.tar.gz \
        bf76df16a495d1fdd9f03fd301f325503d087c20

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    CXXFLAGS=-std=c++14 ./autogen.sh \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --with-gallium-drivers=swr,swrast \
        --enable-swr-native \
        --enable-glx-tls \
        --with-llvm-prefix=/usr/lib/llvm-3.6

    make install
}

function mason_strip_ldflags {
    shift # -L...
    shift # -luv
    echo "$@"
}

function mason_ldflags {
    mason_strip_ldflags $(`mason_pkgconfig` --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"
