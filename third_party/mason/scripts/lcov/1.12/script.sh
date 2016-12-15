#!/usr/bin/env bash

MASON_NAME=lcov
MASON_VERSION=1.12
MASON_LIB_FILE=usr/bin/lcov

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/ltp/lcov-1.12.tar.gz \
        c7470ce9d89bb9c276ef7f461e9ab5b9c9935eff

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    if [[ ${MASON_PLATFORM} == "osx" || ${MASON_PLATFORM} == "ios" ]]; then
        brew install coreutils
        export PATH="/usr/local/opt/coreutils/libexec/gnubin:${PATH}"
    fi
    PREFIX=${MASON_PREFIX} make install
}

function mason_clean {
    make clean
}

mason_run "$@"
