#!/usr/bin/env bash

MASON_NAME=valgrind
MASON_VERSION=3.12.0
MASON_LIB_FILE=bin/valgrind

MASON_IGNORE_OSX_SDK=true
. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://valgrind.org/downloads/valgrind-${MASON_VERSION}.tar.bz2 \
        9d10673077704b02513d216672ecb64d3719f537

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        if [ $(xcode-select -p > /dev/null && echo $?) != 0 ]; then
            sed -i 's@/usr/include/mach@'"$MASON_SDK_PATH"'&@' coregrind/Makefile.am
        fi
        EXTRA_ARGS="--enable-only64bit --build=amd64-darwin"
    fi
    ./autogen.sh
    ./configure ${MASON_HOST_ARG} \
        --prefix=${MASON_PREFIX} \
        --disable-dependency-tracking \
        ${EXTRA_ARGS:-}
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        make install -j${MASON_CONCURRENCY}
    else
        make install-strip -j${MASON_CONCURRENCY}
    fi
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
