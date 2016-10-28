#!/usr/bin/env bash

MASON_NAME=iojs
MASON_VERSION=2.0.1
MASON_LIB_FILE=bin/iojs

. ${MASON_DIR}/mason.sh

function mason_load_source {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        mason_download \
            https://iojs.org/dist/v2.0.1/iojs-v2.0.1-darwin-x64.tar.gz \
            0b6d6a783907f4f4ef84e84af6b1ff9e954aae13
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        mason_download \
            https://iojs.org/dist/v2.0.1/iojs-v2.0.1-linux-x64.tar.gz \
            33ce94f563cc6deaf7bf84d66822be2770b5aad6
    fi

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    mv -v */* ${MASON_PREFIX}
}

mason_run "$@"
