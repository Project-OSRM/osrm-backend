#!/usr/bin/env bash

set -e
set -o pipefail
set -u

export CFLAGS="${MASON_CFLAGS} --sysroot=\${MASON_XC_ROOT}/sysroot -g -DANDROID -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -Wa,--noexecstack -Wformat -Werror=format-security"
export LDFLAGS="${MASON_LDFLAGS} --sysroot=\${MASON_XC_ROOT}/sysroot -Wl,--build-id -Wl,--warn-shared-textrel -Wl,--fatal-warnings -Wl,--no-undefined -Wl,-z,noexecstack -Qunused-arguments -Wl,-z,relro -Wl,-z,now"

# Write CMake toolchain file
CMAKE_TOOLCHAIN="${ROOT}/toolchain.cmake"
echo "set(CMAKE_SYSTEM_NAME Android)" > "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_SYSTEM_VERSION 1)" >> "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_C_COMPILER \"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-clang\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_CXX_COMPILER \"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-clang++\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_RANLIB \"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-ranlib\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_C_FLAGS \"${CFLAGS} \${CMAKE_C_FLAGS}\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_CXX_FLAGS \"${CFLAGS} \${CMAKE_CXX_FLAGS}\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_EXE_LINKER_FLAGS \"${LDFLAGS} \${CMAKE_EXE_LINKER_FLAGS}\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(CMAKE_SHARED_LINKER_FLAGS \"${LDFLAGS} \${CMAKE_SHARED_LINKER_FLAGS}\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(STRIP_COMMAND \"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-strip\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(ANDROID_TOOLCHAIN \"${MASON_ANDROID_TOOLCHAIN}\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(ANDROID_ABI \"${MASON_ANDROID_ABI}\")" >> "${CMAKE_TOOLCHAIN}"
echo "set(ANDROID_NATIVE_API_LEVEL \"${MASON_ANDROID_NDK_API_LEVEL}\")" >> "${CMAKE_TOOLCHAIN}"

# Write shell script toolchain file
SHELL_TOOLCHAIN="${ROOT}/toolchain.sh"
echo "export CC=\"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-clang\"" >> "${SHELL_TOOLCHAIN}"
echo "export CXX=\"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-clang++\"" >> "${SHELL_TOOLCHAIN}"
echo "export LD=\"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-ld\"" >> "${SHELL_TOOLCHAIN}"
echo "export LINK=\"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-clang++\"" >> "${SHELL_TOOLCHAIN}"
echo "export AR=\"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-ar\"" >> "${SHELL_TOOLCHAIN}"
echo "export RANLIB=\"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-ranlib\"" >> "${SHELL_TOOLCHAIN}"
echo "export STRIP=\"\${MASON_XC_ROOT}/bin/${MASON_ANDROID_TOOLCHAIN}-strip\"" >> "${SHELL_TOOLCHAIN}"
echo "export CFLAGS=\"${CFLAGS} \${CFLAGS}\"" >> "${SHELL_TOOLCHAIN}"
echo "export CXXFLAGS=\"${CFLAGS} \${CXXFLAGS}\"" >> "${SHELL_TOOLCHAIN}"
echo "export ANDROID_TOOLCHAIN=\"${MASON_ANDROID_TOOLCHAIN}\"" >> "${SHELL_TOOLCHAIN}"
echo "export ANDROID_ABI=\"${MASON_ANDROID_ABI}\"" >> "${SHELL_TOOLCHAIN}"
echo "export ANDROID_NATIVE_API_LEVEL=\"${MASON_ANDROID_NDK_API_LEVEL}\"" >> "${SHELL_TOOLCHAIN}"
