#!/usr/bin/env bash

MASON_NAME=pixman
MASON_VERSION=0.32.6
MASON_LIB_FILE=lib/libpixman-1.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://cairographics.org/releases/pixman-${MASON_VERSION}.tar.gz \
        ef6a79a704290fa28838d02faad3914fe9cbc895

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --disable-shared \
        --disable-dependency-tracking \
        --disable-mmx \
        --disable-ssse3 \
        --disable-libpng \
        --disable-gtk

    # The -i and -k flags are to workaround osx bug in pixman tests: Undefined symbols for architecture x86_64: "_prng_state
    make -j${MASON_CONCURRENCY} -i -k
    make install -i -k
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    echo -L${MASON_PREFIX}/lib -ljpeg
}

function mason_clean {
    make clean
}

mason_run "$@"
