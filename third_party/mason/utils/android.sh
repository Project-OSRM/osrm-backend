#!/usr/bin/env bash

set -e
set -o pipefail

echo "set(CMAKE_SYSTEM_NAME Android)"
echo "set(CMAKE_SYSTEM_VERSION 1)"
echo "set(CMAKE_CXX_COMPILER \"`which $(${MASON_DIR}/mason env CXX)`\")"
echo "set(CMAKE_C_COMPILER \"`which $(${MASON_DIR}/mason env CC)`\")"
echo "set(ANDROID_JNIDIR \"`${MASON_DIR}/mason env JNIDIR`\")"
echo "set(ANDROID_ABI \"\${ANDROID_JNIDIR}\")"
echo "set(CMAKE_EXE_LINKER_FLAGS \"`${MASON_DIR}/mason env LDFLAGS` \${CMAKE_EXE_LINKER_FLAGS}\")"
echo "set(CMAKE_SHARED_LINKER_FLAGS \"`${MASON_DIR}/mason env LDFLAGS` \${CMAKE_SHARED_LINKER_FLAGS}\")"
echo "set(CMAKE_CXX_FLAGS \"`${MASON_DIR}/mason env CXXFLAGS` \${CMAKE_CXX_FLAGS}\")"
echo "set(CMAKE_C_FLAGS \"`${MASON_DIR}/mason env CPPFLAGS` \${CMAKE_C_FLAGS}\")"
echo "set(STRIP_COMMAND \"`which $(${MASON_DIR}/mason env STRIP)`\")"
