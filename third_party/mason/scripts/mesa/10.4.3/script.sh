#!/usr/bin/env bash

MASON_NAME=mesa
MASON_VERSION=10.4.3
MASON_LIB_FILE=lib/libGL.so
MASON_PKGCONFIG_FILE=lib/pkgconfig/gl.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        ftp://ftp.freedesktop.org/pub/mesa/10.4.3/MesaLib-10.4.3.tar.gz \
        3a0cbced01f666d5d075bd3d6570fa43c91b86be

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/Mesa-10.4.3
}

function mason_compile {
    autoreconf --force --install

    CXXFLAGS=-std=c++11 ./autogen.sh \
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
