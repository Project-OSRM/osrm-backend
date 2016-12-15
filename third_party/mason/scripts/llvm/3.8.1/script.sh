#!/usr/bin/env bash

# dynamically determine the path to this package
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
# dynamically take name of package from directory
MASON_NAME=$(basename $(dirname $HERE))
# dynamically take the version of the package from directory
MASON_VERSION=$(basename $HERE)
# inherit all functions from llvm base
source ${HERE}/../../${MASON_NAME}/base/common.sh

function setup_release() {
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/llvm-${MASON_VERSION}.src.tar.xz"              ${MASON_BUILD_PATH}/                        6de84b7bb71e49ef2764d364c4318e01fda1e1e3
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/cfe-${MASON_VERSION}.src.tar.xz"               ${MASON_BUILD_PATH}/tools/clang             9a05f9c1c8dc865c064782dedbbbfb533c3909ac
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/compiler-rt-${MASON_VERSION}.src.tar.xz"       ${MASON_BUILD_PATH}/projects/compiler-rt    678cbff6e177a18f4e2d0662901a744163da3347
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/libcxx-${MASON_VERSION}.src.tar.xz"            ${MASON_BUILD_PATH}/projects/libcxx         d15220e86eb8480e58a4378a4c977bbb5463fb79
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/libcxxabi-${MASON_VERSION}.src.tar.xz"         ${MASON_BUILD_PATH}/projects/libcxxabi      b7508c64ab8e670062ee57a12ae1e542bcb2bfb4
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/libunwind-${MASON_VERSION}.src.tar.xz"         ${MASON_BUILD_PATH}/projects/libunwind      90c0184ca72e1999fec304f76bfa10340f038ee5
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/lld-${MASON_VERSION}.src.tar.xz"               ${MASON_BUILD_PATH}/tools/lld               416c36ded12ead42dc4739d52eabf22267300883
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/clang-tools-extra-${MASON_VERSION}.src.tar.xz" ${MASON_BUILD_PATH}/tools/clang/tools/extra ea40e36d54dc8c9bb21cbebcc872a3221a2ed685
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/lldb-${MASON_VERSION}.src.tar.xz"              ${MASON_BUILD_PATH}/tools/lldb              c8c38fa9ab92f9021067678f1a1c8f07ea75ac93
    get_llvm_project "https://github.com/include-what-you-use/include-what-you-use/archive/clang_${MAJOR_MINOR}.tar.gz" ${MASON_BUILD_PATH}/tools/clang/tools/include-what-you-use 2b5c5be3c7dcbe555a481083363408399b40e0c8
}

mason_run "$@"
