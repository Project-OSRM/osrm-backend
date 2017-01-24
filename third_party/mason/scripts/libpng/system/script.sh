#!/usr/bin/env bash

MASON_NAME=libpng
MASON_VERSION=system
MASON_SYSTEM_PACKAGE=true

. ${MASON_DIR}/mason.sh

if [ ! $(pkg-config libpng --exists; echo $?) = 0 ]; then
    mason_error "Cannot find libpng with pkg-config"
    exit 1
fi

function mason_system_version {
    # Use host compiler to produce a binary that can run on the host
    HOST_CC=`MASON_PLATFORM= mason env CC`
    HOST_CFLAGS=`MASON_PLATFORM= mason env CFLAGS`

    mkdir -p "${MASON_PREFIX}"
    cd "${MASON_PREFIX}"
    if [ ! -f version ]; then
        echo "#define PNG_VERSION_INFO_ONLY
#include <png.h>
#include <stdio.h>
int main() {
    printf(\"%s\", PNG_LIBPNG_VER_STRING);
    return 0;
}
" > version.c && ${HOST_CC} ${HOST_CFLAGS} version.c $(mason_cflags) -o version
    fi
    ./version
}

function mason_build {
    :
}

function mason_pkgconfig {
    echo 'pkg-config libpng'
}

mason_run "$@"
