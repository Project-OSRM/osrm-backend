#!/usr/bin/env bash

MASON_NAME=apitrace
MASON_VERSION=6a30de1

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/apitrace/apitrace/archive/${MASON_VERSION}.tar.gz \
        622308260cbbe770672ee0753f650aafa7e1a04e

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-6a30de197ad8221e6481510155025a9f93dfd5c3
}

function mason_compile {
    cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo \
                -DCMAKE_INSTALL_PREFIX="${MASON_PREFIX}"
    make -C build
    make -C build install
}

function mason_ldflags {
    :
}

function mason_cflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
