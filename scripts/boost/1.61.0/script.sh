#!/usr/bin/env bash

HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"

# inherit from boost base (used for all boost library packages)
source ${HERE}/base.sh

# this package is the one that is header-only
MASON_NAME=boost
MASON_HEADER_ONLY=true
unset MASON_LIB_FILE

# setup mason env
. ${MASON_DIR}/mason.sh

# source common build functions
source ${HERE}/common.sh

# override default unpacking to just unpack headers
function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/${MASON_VERSION}/boost_${BOOST_VERSION}.tar.bz2 \
        ${BOOST_SHASUM}

    mason_extract_tar_bz2 boost_${BOOST_VERSION}/boost

    MASON_BUILD_PATH=${MASON_ROOT}/.build/boost_${BOOST_VERSION}
}

# override default "compile" target for just the header install
function mason_compile {
    mkdir -p ${MASON_PREFIX}/include
    cp -r ${MASON_ROOT}/.build/boost_${BOOST_VERSION}/boost ${MASON_PREFIX}/include
    
    # work around NDK bug https://code.google.com/p/android/issues/detail?id=79483
    
    patch ${MASON_PREFIX}/include/boost/core/demangle.hpp <<< "19a20,21
> #if !defined(__ANDROID__)
> 
25a28,29
> #endif
> 
"

    # work around https://github.com/Project-OSRM/node-osrm/issues/191
    patch ${MASON_PREFIX}/include/boost/interprocess/detail/os_file_functions.hpp <<< "471c471
<    return ::open(name, (int)mode);
---
>    return ::open(name, (int)mode,S_IRUSR|S_IWUSR);
"

}

mason_run "$@"
