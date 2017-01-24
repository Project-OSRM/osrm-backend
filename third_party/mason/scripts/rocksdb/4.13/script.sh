#!/usr/bin/env bash

MASON_NAME=rocksdb
MASON_VERSION=4.13
MASON_LIB_FILE=lib/librocksdb.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/facebook/${MASON_NAME}/archive/v4.13.tar.gz \
        0f82fd1e08e3c339dab5b19b08e201aebe6dace4

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-4.13
}

function mason_compile {
    # by default -O2 is used for release builds (https://github.com/facebook/rocksdb/commit/1d08140e817d5908889f59046148ed4d3b1039e5)
    # but this is too conservative
    # we want -O3 for best performance
    perl -i -p -e "s/-O2 -fno-omit-frame-pointer/-O3/g;" Makefile
    INSTALL_PATH=${MASON_PREFIX} V=1 make install-static -j${MASON_CONCURRENCY}
    # remove debug symbols (200 MB -> 10 MB)
    strip -S ${MASON_PREFIX}/${MASON_LIB_FILE}
}

function mason_static_libs {
    echo ${MASON_PREFIX}/${MASON_LIB_FILE}
}

function mason_ldflags {
    :
}

mason_run "$@"
