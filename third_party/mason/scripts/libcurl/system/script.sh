#!/usr/bin/env bash

MASON_NAME=libcurl
MASON_VERSION=system
MASON_SYSTEM_PACKAGE=true

. ${MASON_DIR}/mason.sh


if [[ ${MASON_PLATFORM} = 'ios' || ${MASON_PLATFORM} = 'android' ]]; then
    mason_error "Unavailable on platform \"${MASON_PLATFORM}\""
    exit 1
fi

MASON_CFLAGS="-I${MASON_PREFIX}/include"
MASON_LDFLAGS="-L${MASON_PREFIX}/lib"

if [[ ${MASON_PLATFORM} = 'osx' ]]; then
    CURL_INCLUDE_PREFIX="${MASON_SDK_PATH}/usr/include"
    CURL_LIBRARY="${MASON_SDK_PATH}/usr/lib/libcurl.${MASON_DYNLIB_SUFFIX}"
    MASON_LDFLAGS="${MASON_LDFLAGS} -lcurl"
else
    CURL_INCLUDE_PREFIX="`pkg-config libcurl --variable=includedir`"
    CURL_LIBRARY="`pkg-config libcurl --variable=libdir`/libcurl.${MASON_DYNLIB_SUFFIX}"
    MASON_CFLAGS="${MASON_CFLAGS} `pkg-config libcurl --cflags-only-other`"
    MASON_LDFLAGS="${MASON_LDFLAGS} `pkg-config libcurl --libs-only-other --libs-only-l`"
fi


if [ ! -f "${CURL_INCLUDE_PREFIX}/curl/curl.h" ]; then
    mason_error "Can't find header file ${CURL_INCLUDE_PREFIX}/curl/curl.h"
    exit 1
fi

if [ ! -f "${CURL_LIBRARY}" ]; then
    mason_error "Can't find library file ${CURL_LIBRARY}"
    exit 1
fi

function mason_system_version {
    mkdir -p "${MASON_PREFIX}"
    cd "${MASON_PREFIX}"
    if [ ! -f version ]; then
        echo "#include <curl/curl.h>
#include <stdio.h>
int main() {
    printf(\"%s\", curl_version_info(CURLVERSION_NOW)->version);
    return 0;
}
" > version.c && ${CC:-cc} version.c $(mason_cflags) $(mason_ldflags) -o version
    fi
    ./version
}

function mason_build {
    mkdir -p ${MASON_PREFIX}/{include,lib}
    ln -sf ${CURL_INCLUDE_PREFIX}/curl ${MASON_PREFIX}/include/
    ln -sf ${CURL_LIBRARY} ${MASON_PREFIX}/lib/
    echo "build is done and available at ${MASON_PREFIX}/"
}

function mason_cflags {
    echo ${MASON_CFLAGS}
}

function mason_ldflags {
    echo ${MASON_LDFLAGS}
}

mason_run "$@"
