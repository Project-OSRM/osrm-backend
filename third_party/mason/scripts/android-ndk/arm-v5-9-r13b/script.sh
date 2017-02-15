#!/usr/bin/env bash

MASON_NAME=android-ndk
MASON_VERSION=$(basename $(dirname "${BASH_SOURCE[0]}"))
MASON_LIB_FILE=

export MASON_ANDROID_TOOLCHAIN="arm-linux-androideabi"
export MASON_CFLAGS="-target armv5te-none-linux-androideabi -march=armv5te -mtune=xscale -msoft-float -fno-integrated-as -mthumb"
export MASON_LDFLAGS="-Wl,--exclude-libs,libunwind.a"
export MASON_ANDROID_ABI="armeabi"
export MASON_ANDROID_NDK_ARCH="arm"

. ${MASON_DIR}/scripts/android-ndk/script-${MASON_VERSION##*-}.sh
