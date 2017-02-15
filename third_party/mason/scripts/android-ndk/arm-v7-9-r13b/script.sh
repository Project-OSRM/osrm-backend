#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=$(basename $(dirname "${BASH_SOURCE[0]}"))
MASON_LIB_FILE=

export MASON_ANDROID_TOOLCHAIN="arm-linux-androideabi"
export MASON_CFLAGS="-target armv7-none-linux-androideabi -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -fno-integrated-as -mthumb"
export MASON_LDFLAGS="-Wl,--fix-cortex-a8 -Wl,--exclude-libs,libunwind.a"
export MASON_ANDROID_ABI="armeabi-v7a"
export MASON_ANDROID_NDK_ARCH="arm"

. ${MASON_DIR}/scripts/android-ndk/script-${MASON_VERSION##*-}.sh
