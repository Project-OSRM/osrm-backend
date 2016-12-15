#!/usr/bin/env bash

export MASON_HOST_ARG="--host=arm-linux-gnueabihf"
export MASON_CMAKE_TOOLCHAIN="-DMASON_XC_ROOT=${MASON_XC_ROOT} -DCMAKE_TOOLCHAIN_FILE=${MASON_XC_ROOT}/toolchain.cmake"

export CXX="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-g++"
export CC="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-gcc"
export LD="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-ld"
export LINK="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-g++"
export AR="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-ar"
export RANLIB="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-ranlib"
export STRIP="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-strip"
export STRIP="${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabihf-strip"
export CFLAGS="-mtune=cortex-a9 -march=armv7-a -mfloat-abi=hard -mfpu=neon-fp16 ${CFLAGS}"
