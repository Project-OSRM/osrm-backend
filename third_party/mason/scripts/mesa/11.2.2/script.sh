#!/usr/bin/env bash

MASON_NAME=mesa
MASON_VERSION=11.2.2

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        ftp://ftp.freedesktop.org/pub/mesa/${MASON_VERSION}/mesa-${MASON_VERSION}.tar.gz \
        1b69da0b8d72d59c62c5dc6ff0d71d79e1a8db9d

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/mesa-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-shared \
        --with-gallium-drivers=swrast \
        --disable-dri \
        --disable-egl \
        --enable-xlib-glx \
        --enable-glx-tls \
        --with-llvm-prefix=/usr/lib/llvm-3.4

    make
    make install
}

function mason_clean {
    make clean
}

mason_run "$@"
