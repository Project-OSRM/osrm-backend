#!/usr/bin/env bash

MASON_NAME=node
MASON_VERSION=0.12.0
MASON_LIB_FILE=bin/node

. ${MASON_DIR}/mason.sh

function mason_load_source {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        mason_download \
            https://nodejs.org/dist/v0.12.0/node-v0.12.0-darwin-x64.tar.gz \
            202d45f836ec45db11a58c4a3ced9774ea2144a2
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        mason_download \
            https://nodejs.org/dist/v0.12.0/node-v0.12.0-linux-x64.tar.gz \
            0cc11d3465ae5b57c181bdb6064e2ba618910d41
    fi

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    mv -v */* ${MASON_PREFIX}
}

mason_run "$@"
