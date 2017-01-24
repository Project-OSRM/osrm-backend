#!/usr/bin/env bash

export MASON_HOST_ARG="--host=arm-linux-gnueabi"
export MASON_CMAKE_TOOLCHAIN="-DMASON_XC_ROOT=${MASON_XC_ROOT} -DCMAKE_TOOLCHAIN_FILE=${MASON_XC_ROOT}/toolchain.cmake"

export CXX="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-g++"
export CC="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-gcc"
export LD="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-ld"
export LINK="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-g++"
export AR="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-ar"
export RANLIB="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-ranlib"
export STRIP="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-strip"
export CFLAGS="-mtune=cortex-a9 -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ${CFLAGS}"
