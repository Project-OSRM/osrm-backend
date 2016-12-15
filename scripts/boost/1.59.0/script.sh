#!/usr/bin/env bash

MASON_NAME=boost
MASON_VERSION=1.59.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

BOOST_ROOT=${MASON_PREFIX}

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/1.59.0/boost_1_59_0.tar.bz2 \
        ff2e48f4d7e3c4b393d41e07a2f5d923b990967d

    mason_extract_tar_bz2 boost_1_59_0/boost

    MASON_BUILD_PATH=${MASON_ROOT}/.build/boost_1_59_0
}

function mason_prefix {
    echo "${BOOST_ROOT}"
}

function mason_compile {
    mkdir -p ${BOOST_ROOT}/include
    mv ${MASON_ROOT}/.build/boost_1_59_0/boost ${BOOST_ROOT}/include
    
    # work around NDK bug https://code.google.com/p/android/issues/detail?id=79483
    
    patch ${BOOST_ROOT}/include/boost/core/demangle.hpp <<< "19a20,21
> #if !defined(__ANDROID__)
> 
25a28,29
> #endif
> 
"

}

function mason_cflags {
    echo "-I${BOOST_ROOT}/include"
}

function mason_ldflags {
    :
}

mason_run "$@"
