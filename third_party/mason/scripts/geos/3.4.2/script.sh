#!/usr/bin/env bash

MASON_NAME=geos
MASON_VERSION=3.4.2
MASON_LIB_FILE=lib/libgeos.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://download.osgeo.org/geos/${MASON_NAME}-${MASON_VERSION}.tar.bz2 \
        b248842dee2afa6e944693c21571a2999dfafc5a

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
    curl --retry 3 -s -f -# -L \
      https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
      -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)
    patch -N -p1 < ./patch.diff

    # note: we put ${STDLIB_CXXFLAGS} into CXX instead of LDFLAGS due to libtool oddity:
    # http://stackoverflow.com/questions/16248360/autotools-libtool-link-library-with-libstdc-despite-stdlib-libc-option-pass
    if [[ $(uname -s) == 'Darwin' ]]; then
        CXX="${CXX} -stdlib=libc++ -std=c++11"
    fi
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --disable-shared --enable-static \
        --disable-dependency-tracking
    make -j${MASON_CONCURRENCY} install
}

function mason_clean {
    make clean
}

mason_run "$@"
