#!/usr/bin/env bash

MASON_NAME=node
MASON_VERSION=0.10.35
MASON_LIB_FILE=bin/node

. ${MASON_DIR}/mason.sh

function mason_load_source {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        mason_download \
            https://nodejs.org/dist/v0.10.35/node-v0.10.35-darwin-x64.tar.gz \
            f0311c1291cafe98649e5733210792f7d57cbcd1
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        mason_download \
            https://nodejs.org/dist/v0.10.35/node-v0.10.35-linux-x64.tar.gz \
            ffcb8592e9a2556ea0a25284d5ab5fe608f344c0
    fi

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    mv -v */* ${MASON_PREFIX}
}

mason_run "$@"
