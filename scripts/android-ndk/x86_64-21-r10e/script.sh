#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=x86_64-21-r10e
MASON_LIB_FILE=

. ${MASON_DIR}/mason.sh

function mason_load_source {
    if [ ${MASON_PLATFORM} = 'osx' ]; then
        mason_download \
            http://dl.google.com/android/ndk/android-ndk-r10e-darwin-x86_64.bin \
            dea2dd3939eea3289cab075804abb153014b78d3
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        mason_download \
            http://dl.google.com/android/ndk/android-ndk-r10e-linux-x86_64.bin \
            285606ba6882d27d99ed469fc5533cc3c93967f5
    fi

    mason_setup_build_dir
    chmod +x ../.cache/${MASON_SLUG}
    ../.cache/${MASON_SLUG} > /dev/null

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/android-ndk-r10e
}

function mason_compile {
    rm -rf ${MASON_PREFIX}
    mkdir -p ${MASON_PREFIX}

    ${MASON_BUILD_PATH}/build/tools/make-standalone-toolchain.sh \
          --toolchain="x86_64-4.9" \
          --llvm-version="3.6" \
          --package-dir="${MASON_BUILD_PATH}/package-dir/" \
          --install-dir="${MASON_PREFIX}" \
          --stl="libcxx" \
          --arch="x86_64" \
          --platform="android-21"
}

function mason_clean {
    make clean
}

mason_run "$@"
