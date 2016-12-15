#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=arm64-21-r13b
MASON_LIB_FILE=

. ${MASON_DIR}/mason.sh

function mason_load_source {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        mason_download \
            http://dl.google.com/android/repository/android-ndk-r13b-darwin-x86_64.zip \
            b822dd239f63cd2e1e72c823c41bd732da2e5ad6
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        mason_download \
            http://dl.google.com/android/repository/android-ndk-r13b-linux-x86_64.zip \
            b95dd1fba5096ca3310a67e90b2a5a8aca3ddec7
    fi

    mason_setup_build_dir
    rm -rf ./android-ndk-r13b
    unzip -q ../.cache/${MASON_SLUG} $@

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/android-ndk-r13b
}

function mason_compile {
    rm -rf ${MASON_PREFIX}
    mkdir -p ${MASON_PREFIX}

    ${MASON_BUILD_PATH}/build/tools/make_standalone_toolchain.py \
        --force \
        --arch arm64 \
        --api 21 \
        --stl libc++ \
        --install-dir "${MASON_PREFIX}"

    # NDK r12 ships with .so files which are preferred when linking, but cause
    # errors on devices when it's not present.
    find "${MASON_PREFIX}" -name "libstdc++.so" -delete
}

function mason_clean {
    make clean
}

mason_run "$@"
