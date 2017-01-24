#!/usr/bin/env bash

MASON_NAME=mesa
MASON_VERSION=10.5.4-dbg
MASON_LIB_FILE=lib/libGL.so
MASON_PKGCONFIG_FILE=lib/pkgconfig/gl.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        ftp://ftp.freedesktop.org/pub/mesa/10.5.4/mesa-10.5.4.tar.gz \
        2c87044700a738d23133477864c62f194aa9daba

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/mesa-10.5.4
}

function mason_compile {
    autoreconf --force --install

    CFLAGS='-g -DDEBUG' CXXFLAGS='-g -DDEBUG -std=c++11' ./autogen.sh \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-shared \
        --with-gallium-drivers=svga,swrast \
        --disable-dri \
        --enable-xlib-glx \
        --enable-glx-tls \
        --with-llvm-prefix=/usr/lib/llvm-3.4 \
        --without-va

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
