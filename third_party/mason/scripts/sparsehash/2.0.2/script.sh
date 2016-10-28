#!/usr/bin/env bash

MASON_NAME=sparsehash
MASON_VERSION=2.0.2
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://sparsehash.googlecode.com/files/sparsehash-2.0.2.tar.gz \
        700205d99da682a3d70512cd28aeea2805d39aab

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
    curl --retry 3 -s -f -# -L \
      https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
      -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)    
    patch -N -p1 < ./patch.diff
    ./configure --prefix=${MASON_PREFIX} ${MASON_HOST_ARG} \
    --enable-static --disable-shared \
    --disable-dependency-tracking
    make  -j${MASON_CONCURRENCY}
    make install    
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
