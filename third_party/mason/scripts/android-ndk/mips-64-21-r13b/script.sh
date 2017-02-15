#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=$(basename $(dirname "${BASH_SOURCE[0]}"))
MASON_LIB_FILE=

export MASON_ANDROID_TOOLCHAIN="mips64el-linux-android"
export MASON_CFLAGS="-target mips64el-none-linux-android"
export MASON_LDFLAGS=""
export MASON_ANDROID_ABI="mips64"
export MASON_ANDROID_NDK_ARCH="mips64"

. ${MASON_DIR}/scripts/android-ndk/script-${MASON_VERSION##*-}.sh

