#!/usr/bin/env bash

MASON_NAME=harfbuzz
MASON_VERSION=1.4.2
MASON_LIB_FILE=lib/libharfbuzz.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/harfbuzz.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.freedesktop.org/software/harfbuzz/release/harfbuzz-${MASON_VERSION}.tar.bz2 \
        d8b08c8d792500f414472c8a54f69b08aabb06b4

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    MASON_PLATFORM= ${MASON_DIR}/mason install ragel 6.9
    export PATH=$(MASON_PLATFORM= ${MASON_DIR}/mason prefix ragel 6.9)/bin:$PATH
    if [[ ! `which pkg-config` ]]; then
        echo "harfbuzz configure needs pkg-config, please install pkg-config"
        exit 1
    fi
}

function mason_compile {
    # Note CXXFLAGS overrides the harfbuzz default with is `-O2 -g`
    export CXXFLAGS="${CXXFLAGS} -O3 -DNDEBUG"
    export CFLAGS="${CFLAGS} -O3 -DNDEBUG"

    ./configure --prefix=${MASON_PREFIX} ${MASON_HOST_ARG} \
     --enable-static \
     --disable-shared \
     --disable-dependency-tracking \
     --with-freetype=no \
     --with-ucdn=yes \
     --with-icu=no \
     --with-cairo=no \
     --with-glib=no \
     --with-gobject=no \
     --with-graphite2=no \
     --with-fontconfig=no \
     --with-uniscribe=no \
     --with-directwrite=no \
     --with-coretext=no

    make -j${MASON_CONCURRENCY} V=1
    make install
}

function mason_ldflags {
    : # We're only using the full path to the archive, which is output in static_libs
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_clean {
    make clean
}

mason_run "$@"
