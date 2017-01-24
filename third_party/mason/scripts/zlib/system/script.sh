#!/usr/bin/env bash

MASON_NAME=zlib
MASON_VERSION=system
MASON_SYSTEM_PACKAGE=true

. ${MASON_DIR}/mason.sh


if [[ ${MASON_PLATFORM} = 'ios' ]]; then
    MASON_CFLAGS=""
    MASON_LDFLAGS="-lz"
else
    MASON_CFLAGS="-I${MASON_PREFIX}/include"
    MASON_LDFLAGS="-L${MASON_PREFIX}/lib"

    if [[ ${MASON_PLATFORM} = 'osx' || ${MASON_PLATFORM} = 'android' ]]; then
        ZLIB_INCLUDE_PREFIX="${MASON_SDK_PATH}/usr/include"
        if [[ -d "${MASON_SDK_PATH}/usr/lib64" ]]; then
            ZLIB_LIBRARY="${MASON_SDK_PATH}/usr/lib64/libz.${MASON_DYNLIB_SUFFIX}"
        else
            ZLIB_LIBRARY="${MASON_SDK_PATH}/usr/lib/libz.${MASON_DYNLIB_SUFFIX}"
        fi
        MASON_LDFLAGS="${MASON_LDFLAGS} -lz"
    else
        ZLIB_INCLUDE_PREFIX="${MASON_SDK_PATH}`pkg-config zlib --variable=includedir`"
        ZLIB_LIBRARY="${MASON_SDK_PATH}`pkg-config zlib --variable=libdir`/libz.${MASON_DYNLIB_SUFFIX}"
        MASON_CFLAGS="${MASON_CFLAGS} `pkg-config zlib --cflags-only-other`"
        MASON_LDFLAGS="${MASON_LDFLAGS} `pkg-config zlib --libs-only-other --libs-only-l`"
    fi

    if [ ! -f "${ZLIB_INCLUDE_PREFIX}/zlib.h" -a ! -h  "${ZLIB_INCLUDE_PREFIX}/zlib.h" ]; then
        mason_error "Can't find header file ${ZLIB_INCLUDE_PREFIX}/zlib.h"
        exit 1
    fi

    if [ ! -f "${ZLIB_LIBRARY}" -a ! -h "${ZLIB_LIBRARY}" ]; then
        mason_error "Can't find library file ${ZLIB_LIBRARY}"
        exit 1
    fi
fi

function mason_system_version {
    if [[ ${MASON_PLATFORM} = 'ios' ]]; then
        FLAGS="-I${MASON_SDK_PATH}/usr/include"
    else
        FLAGS=$(mason_cflags)
    fi

    mkdir -p "${MASON_PREFIX}"
    cd "${MASON_PREFIX}"
    if [ ! -f version ]; then
        echo "#include <zlib.h>
#include <stdio.h>
int main() {
    printf(\"%s\", ZLIB_VERSION);
    return 0;
}
" > version.c && cc version.c ${FLAGS} -o version
    fi
    ./version
}

function mason_build {
    if [[ ${MASON_PLATFORM} != 'ios' ]]; then
        mkdir -p ${MASON_PREFIX}/{include,lib}
        ln -sf ${ZLIB_INCLUDE_PREFIX}/z*.h ${MASON_PREFIX}/include/
        ln -sf ${ZLIB_LIBRARY} ${MASON_PREFIX}/lib/
    fi
    VERSION=$(mason_system_version)
}

function mason_cflags {
    echo ${MASON_CFLAGS}
}

function mason_ldflags {
    echo ${MASON_LDFLAGS}
}

mason_run "$@"
