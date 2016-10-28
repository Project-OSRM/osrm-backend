#!/usr/bin/env bash

MASON_NAME=rapidjson
MASON_VERSION=2016-07-20-369de87
MASON_HEADER_ONLY=true

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
    https://github.com/miloyip/rapidjson/archive/369de87e5d7da05786731a712f25ab9b46c4b0ce.tar.gz \
    e2b9b08ee980ab82a4ced50a36d2bd12ac1a5dc4
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/rapidjson-369de87e5d7da05786731a712f25ab9b46c4b0ce
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}
    cp -rv include ${MASON_PREFIX}
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    : # We're only using the full path to the archive, which is output in static_libs
}

function mason_clean {
    make clean
}

mason_run "$@"
