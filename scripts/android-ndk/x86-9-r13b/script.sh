#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=$(basename $(dirname "${BASH_SOURCE[0]}"))
MASON_LIB_FILE=

export MASON_ANDROID_TOOLCHAIN="i686-linux-android"
export MASON_CFLAGS="-target i686-none-linux-android"
export MASON_LDFLAGS=""
export MASON_ANDROID_ABI="x86"
export MASON_ANDROID_NDK_ARCH="x86"

. ${MASON_DIR}/scripts/android-ndk/script-${MASON_VERSION##*-}.sh
