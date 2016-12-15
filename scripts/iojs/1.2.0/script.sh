#!/usr/bin/env bash

MASON_NAME=iojs
MASON_VERSION=1.2.0
MASON_LIB_FILE=bin/iojs

. ${MASON_DIR}/mason.sh

function mason_load_source {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        mason_download \
            https://iojs.org/dist/v1.2.0/iojs-v1.2.0-darwin-x64.tar.gz \
            15c553a35abb84085f993e605b83a6b924e22f3c
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        mason_download \
            https://iojs.org/dist/v1.2.0/iojs-v1.2.0-linux-x64.tar.gz \
            054234cb47ba4a3b3826a892836760b107596a57
    fi

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    mv -v */* ${MASON_PREFIX}
}

mason_run "$@"
