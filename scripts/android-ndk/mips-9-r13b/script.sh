#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=$(basename $(dirname "${BASH_SOURCE[0]}"))
MASON_LIB_FILE=

export MASON_ANDROID_TOOLCHAIN="mipsel-linux-android"
export MASON_CFLAGS="-target mipsel-none-linux-android -mips32"
export MASON_LDFLAGS=""
export MASON_ANDROID_ABI="mips"
export MASON_ANDROID_NDK_ARCH="mips"

. ${MASON_DIR}/scripts/android-ndk/script-${MASON_VERSION##*-}.sh
