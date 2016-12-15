#!/usr/bin/env bash

MASON_NAME=leveldb
MASON_VERSION=a7bff69
MASON_LIB_FILE=lib/libleveldb.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/leveldb/archive/a7bff697baa062c8f6b8fb760eacf658712b611a.tar.gz \
        f9fb5e3c97ab59e2a8c24c68eb7af85f17a370ff

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-a7bff697baa062c8f6b8fb760eacf658712b611a
}

function mason_compile {
    # note: OPT is set in the Makefile and defaults to '-O2 -DNDEBUG'
    # (dane) we should use -O3 for the fastest code - I presume -O2
    # is being used as safe default since some old compilers were buggy
    # with -O3 back in the day...
    OPT="-O3 -DNDEBUG" make out-static/libleveldb.a -j${MASON_CONCURRENCY}
    # leveldb lacks an install target
    # https://github.com/google/leveldb/pull/2
    mkdir -p ${MASON_PREFIX}/include/
    mkdir -p ${MASON_PREFIX}/lib/
    cp -r include/leveldb ${MASON_PREFIX}/include/leveldb
    cp out-static/libleveldb.a ${MASON_PREFIX}/lib/
}

function mason_ldflags {
    echo -L${MASON_PREFIX}/lib -lleveldb
}

function mason_clean {
    make clean
}

mason_run "$@"
