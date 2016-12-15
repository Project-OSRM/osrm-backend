#!/usr/bin/env bash

MASON_NAME=sqlite
MASON_VERSION=system
MASON_SYSTEM_PACKAGE=true

. ${MASON_DIR}/mason.sh


if [[ ${MASON_PLATFORM} = 'android' ]]; then
    mason_error "Unavailable on platform \"${MASON_PLATFORM}\""
    exit 1
elif [[ ${MASON_PLATFORM} = 'ios' ]]; then
    MASON_CFLAGS=""
    MASON_LDFLAGS="-lsqlite3"
else
    MASON_CFLAGS="-I${MASON_PREFIX}/include"
    MASON_LDFLAGS="-L${MASON_PREFIX}/lib"
    SQLITE_INCLUDE_PREFIX="`pkg-config sqlite3 --variable=includedir`"
    SQLITE_LIBRARY="`pkg-config sqlite3 --variable=libdir`/libsqlite3.${MASON_DYNLIB_SUFFIX}"
    MASON_CFLAGS="${MASON_CFLAGS} `pkg-config sqlite3 --cflags-only-other`"
    MASON_LDFLAGS="${MASON_LDFLAGS} `pkg-config sqlite3 --libs-only-other --libs-only-l`"

    if [ ! -f "${SQLITE_INCLUDE_PREFIX}/sqlite3.h" ]; then
        mason_error "Can't find header file ${SQLITE_INCLUDE_PREFIX}/sqlite3.h"
        exit 1
    fi

    if [ ! -f "${SQLITE_LIBRARY}" ]; then
        mason_error "Can't find library file ${SQLITE_LIBRARY}"
        exit 1
    fi
fi

function mason_system_version {
    if [[ ${MASON_PLATFORM} = 'ios' ]]; then
        FLAGS="-I${MASON_SDK_PATH}/usr/include -Wp,-w"
    else
        FLAGS=$(mason_cflags)
    fi

    mkdir -p "${MASON_PREFIX}"
    cd "${MASON_PREFIX}"
    if [ ! -f version ]; then
        echo "#include <sqlite3.h>
#include <stdio.h>
int main() {
    printf(\"%s\", SQLITE_VERSION);
    return 0;
}
" > version.c && cc version.c ${FLAGS} -o version
    fi
    ./version
}

function mason_build {
    if [[ ! -z ${SQLITE_INCLUDE_PREFIX} ]]; then
        mkdir -p ${MASON_PREFIX}/include
        ln -sf ${SQLITE_INCLUDE_PREFIX}/sqlite3.h ${MASON_PREFIX}/include/
    fi
    if [[ ! -z ${SQLITE_LIBRARY} ]]; then
        mkdir -p ${MASON_PREFIX}/lib
        ln -sf ${SQLITE_LIBRARY} ${MASON_PREFIX}/lib/
    fi
}

function mason_cflags {
    echo ${MASON_CFLAGS}
}

function mason_ldflags {
    echo ${MASON_LDFLAGS}
}

mason_run "$@"
