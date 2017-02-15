#!/usr/bin/env bash

MASON_NAME=harfbuzz
MASON_VERSION=0.9.40
MASON_LIB_FILE=lib/libharfbuzz.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/harfbuzz.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.freedesktop.org/software/harfbuzz/release/harfbuzz-${MASON_VERSION}.tar.bz2 \
        a685da85d38c37fd27603165642fc09feb7ae7c1

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    FREETYPE_VERSION="2.5.5"
    ${MASON_DIR}/mason install freetype ${FREETYPE_VERSION}
    MASON_FREETYPE=$(${MASON_DIR}/mason prefix freetype ${FREETYPE_VERSION})
    MASON_PLATFORM= ${MASON_DIR}/mason install ragel 6.9
    export PATH=$(MASON_PLATFORM= ${MASON_DIR}/mason prefix ragel 6.9)/bin:$PATH
    export PKG_CONFIG_PATH="$(${MASON_DIR}/mason prefix freetype ${FREETYPE_VERSION})/lib/pkgconfig":$PKG_CONFIG_PATH
    export C_INCLUDE_PATH="${MASON_FREETYPE}/include/freetype2"
    export CPLUS_INCLUDE_PATH="${MASON_FREETYPE}/include/freetype2"
    export LIBRARY_PATH="${MASON_FREETYPE}/lib"
    if [[ ! `which pkg-config` ]]; then
        echo "harfbuzz configure needs pkg-config, please install pkg-config"
        exit 1
    fi
}

function mason_compile {
    export FREETYPE_CFLAGS="-I${MASON_FREETYPE}/include/freetype2"
    export FREETYPE_LIBS="-L${MASON_FREETYPE}/lib -lfreetype -lz"
    # Note CXXFLAGS overrides the harbuzz default with is `-O2 -g`
    export CXXFLAGS="${CXXFLAGS} ${FREETYPE_CFLAGS} -O3 -DNDEBUG"
    export CFLAGS="${CFLAGS} ${FREETYPE_CFLAGS} -O3 -DNDEBUG"
    export LDFLAGS="${LDFLAGS} ${FREETYPE_LIBS}"

    mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
    curl --retry 3 -s -f -# -L \
      https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
      -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)
    patch -N -p1 < ./patch.diff

    ./configure --prefix=${MASON_PREFIX} ${MASON_HOST_ARG} \
     --enable-static \
     --disable-shared \
     --disable-dependency-tracking \
     --with-icu=no \
     --with-cairo=no \
     --with-glib=no \
     --with-gobject=no \
     --with-graphite2=no \
     --with-freetype \
     --with-uniscribe=no \
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
