#!/usr/bin/env bash

MASON_NAME=twemproxy
MASON_VERSION=0.4.1
MASON_LIB_FILE=sbin/nutcracker

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/twitter/${MASON_NAME}/archive/v${MASON_VERSION}.tar.gz \
        8af659bf54240aecc2f86cdd9ba3ddcc9c35f1f0

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}


function mason_compile {
    autoreconf -ivf
    # NOTE: CFLAGS overrides internal default
    # which in this case is desirable because it allows us to override the internal
    # default of -O2
    export CFLAGS="${CFLAGS} -O3 -DNDEBUG"
    ./configure \
      --prefix=${MASON_PREFIX} \
      --disable-debug --disable-dependency-tracking
    make -j${MASON_CONCURRENCY}
    make install
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

mason_run "$@"
