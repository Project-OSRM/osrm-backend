#!/usr/bin/env bash

MASON_NAME=boost
MASON_VERSION=1.58.0
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

BOOST_ROOT=${MASON_PREFIX}

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/1.58.0/boost_1_58_0.tar.bz2 \
        43e46651e762e4daf72a5d21dca86ae151e65378

    mason_extract_tar_bz2 boost_1_58_0/boost

    MASON_BUILD_PATH=${MASON_ROOT}/.build/boost_1_58_0
}

function mason_prefix {
    echo "${BOOST_ROOT}"
}

function mason_compile {
    mkdir -p ${BOOST_ROOT}/include
    mv ${MASON_ROOT}/.build/boost_1_58_0/boost ${BOOST_ROOT}/include
    
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
