#!/usr/bin/env bash

MASON_NAME=node
MASON_VERSION=0.10.36
MASON_LIB_FILE=bin/node

. ${MASON_DIR}/mason.sh

function mason_load_source {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        mason_download \
            https://nodejs.org/dist/v0.10.36/node-v0.10.36-darwin-x64.tar.gz \
            bead5971c06fb58ac5a84f81187c5ec45dfc2c15
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        mason_download \
            https://nodejs.org/dist/v0.10.36/node-v0.10.36-linux-x64.tar.gz \
            f4c2b6f439671adc3ede4c435ead261cece7a05d
    fi

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    mv -v */* ${MASON_PREFIX}
}

mason_run "$@"
