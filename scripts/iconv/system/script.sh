#!/usr/bin/env bash

MASON_NAME=iconv
MASON_VERSION=system
MASON_SYSTEM_PACKAGE=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    :
}

function mason_system_version {
    mkdir -p "${MASON_PREFIX}"
    cd "${MASON_PREFIX}"
    if [ ! -f version ]; then
        echo "#include <iconv.h>
#include <stdio.h>
#include <assert.h>
int main() {
    printf(\"%d.%d\",_libiconv_version >> 8, _libiconv_version & 0xff);
    return 0;
}
" > version.c && cc version.c -I${MASON_PREFIX}/include/ -L${MASON_PREFIX}/lib -liconv -o version
    fi
    ./version
}

if [[ ${MASON_PLATFORM} = 'osx' || ${MASON_PLATFORM} = 'ios' ]]; then
    MASON_HEADER_FILE="${MASON_SDK_PATH}/usr/include/iconv.h"
    if [ ! -f "${MASON_HEADER_FILE}" ]; then
        mason_error "Can't find header file ${MASON_HEADER_FILE}"
        exit 1
    fi

    MASON_LIBRARY_FILE="${MASON_SDK_PATH}/usr/lib/libiconv.dylib"
    if [ ! -f "${MASON_LIBRARY_FILE}" ]; then
        mason_error "Can't find library file ${MASON_LIBRARY_FILE}"
        exit 1
    fi
    MASON_CFLAGS="-I${MASON_PREFIX}/include/"
    MASON_LDFLAGS="-L${MASON_PREFIX}/lib -liconv"
elif [[ ${MASON_PLATFORM} = 'android' ]]; then
    MASON_HEADER_FILE="${MASON_SDK_PATH}/usr/include/iconv.h"
    if [ ! -f "${MASON_HEADER_FILE}" ]; then
        mason_error "Can't find header file ${MASON_HEADER_FILE}"
        exit 1
    fi

    MASON_LIBRARY_FILE="${MASON_SDK_PATH}/usr/lib/libiconv.so"
    if [ ! -f "${MASON_LIBRARY_FILE}" ]; then
        mason_error "Can't find library file ${MASON_LIBRARY_FILE}"
        exit 1
    fi

    MASON_CFLAGS="-I${MASON_PREFIX}/include/"
    MASON_LDFLAGS="-L${MASON_PREFIX}/lib -liconv"
else
    MASON_CFLAGS="-I${MASON_PREFIX}/include/"
    MASON_LDFLAGS="-L${MASON_PREFIX}/lib -liconv"
fi

function mason_compile {
    mkdir -p ${MASON_PREFIX}/lib/
    mkdir -p ${MASON_PREFIX}/include/
    if [[ ${MASON_PLATFORM} = 'osx' || ${MASON_PLATFORM} = 'ios' ]]; then
        ln -sf ${MASON_SDK_PATH}/usr/include/iconv.h ${MASON_PREFIX}/include/iconv.h
        ln -sf ${MASON_SDK_PATH}/usr/lib/libiconv.dylib ${MASON_PREFIX}/lib/libiconv.dylib
    elif [[ ${MASON_PLATFORM} = 'android' ]]; then
        ln -sf ${MASON_SDK_PATH}/usr/include/iconv.h ${MASON_PREFIX}/include/iconv.h
        ln -sf ${MASON_SDK_PATH}/usr/lib/libiconv.dylib ${MASON_PREFIX}/lib/libiconv.dylib
    elif [[ -d /usr/include/iconv.h ]]; then
        ln -sf /usr/include/iconv.h ${MASON_PREFIX}/include/iconv.h
        ln -sf /usr/lib/libiconv.so ${MASON_PREFIX}/lib/libiconv.so
    fi
}

function mason_cflags {
    echo ${MASON_CFLAGS}
}

function mason_ldflags {
    echo ${MASON_LDFLAGS}
}

mason_run "$@"
