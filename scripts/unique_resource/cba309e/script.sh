#!/usr/bin/env bash

MASON_NAME=unique_resource
MASON_VERSION=cba309e
MASON_HEADER_ONLY=true

GIT_HASH="cba309e92ec79a95be2aa5a324a688a06af8d40a"

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/okdshin/${MASON_NAME}/archive/${GIT_HASH}.tar.gz \
        1423b932d80d39250fd0c5715b165bb0efa63883

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${GIT_HASH}
}

function mason_compile {
    mkdir -p ${MASON_PREFIX}/include/
    cp -r unique_resource.hpp ${MASON_PREFIX}/include/
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    :
}


mason_run "$@"
