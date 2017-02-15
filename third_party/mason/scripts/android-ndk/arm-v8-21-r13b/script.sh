#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=$(basename $(dirname "${BASH_SOURCE[0]}"))
MASON_LIB_FILE=

export MASON_ANDROID_TOOLCHAIN="aarch64-linux-android"
export MASON_CFLAGS="-target aarch64-none-linux-android"
export MASON_LDFLAGS=""
export MASON_ANDROID_ABI="arm64-v8a"
export MASON_ANDROID_NDK_ARCH="arm64"

. ${MASON_DIR}/scripts/android-ndk/script-${MASON_VERSION##*-}.sh
