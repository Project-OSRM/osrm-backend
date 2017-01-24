#!/usr/bin/env bash

export MASON_HOST_ARG="--host=i686-pc-linux-gnu"
export MASON_CMAKE_TOOLCHAIN="-DMASON_XC_ROOT=${MASON_XC_ROOT} -DCMAKE_TOOLCHAIN_FILE=${MASON_XC_ROOT}/toolchain.cmake"

export CXX="${MASON_XC_ROOT}/root/bin/i686-pc-linux-gnu-g++"
export CC="${MASON_XC_ROOT}/root/bin/i686-pc-linux-gnu-gcc"
export LD="${MASON_XC_ROOT}/root/bin/i686-pc-linux-gnu-ld"
export LINK="${MASON_XC_ROOT}/root/bin/i686-pc-linux-gnu-g++"
export AR="${MASON_XC_ROOT}/root/bin/i686-pc-linux-gnu-ar"
export RANLIB="${MASON_XC_ROOT}/root/bin/i686-pc-linux-gnu-ranlib"
export STRIP="${MASON_XC_ROOT}/root/bin/i686-pc-linux-gnu-strip"
export CFLAGS="-mtune=i686 -march=i686 ${CFLAGS}"
