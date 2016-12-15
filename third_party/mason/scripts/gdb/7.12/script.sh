#!/usr/bin/env bash

MASON_NAME=gdb
MASON_VERSION=7.12
MASON_LIB_FILE=bin/gdb

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://ftp.gnu.org/gnu/gdb/${MASON_NAME}-${MASON_VERSION}.tar.gz \
        d66badff54a8a47ed26d99f07e26c805572e9639

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    # set -fpermissive to try to work around
    # location.c:527:16: error: comparison between pointer and integer ('const char *' and 'int')
    # || *argp == '\0'
    export CFLAGS="${CFLAGS} -fpermissive"
    export CXXFLAGS="${CXXFLAGS} -fpermissive"
    ./configure --prefix=${MASON_PREFIX} \
     --enable-static \
     --disable-debug \
     --disable-dependency-tracking \
     --without-guile \
     --without-python \
     --with-system-zlib

    make -j${MASON_CONCURRENCY} V=1
    make install
}

function mason_clean {
    make clean
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
