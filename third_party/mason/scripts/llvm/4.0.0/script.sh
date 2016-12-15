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
    get_llvm_project "http://llvm.org/git/llvm.git"              ${MASON_BUILD_PATH}
    get_llvm_project "http://llvm.org/git/clang.git"             ${MASON_BUILD_PATH}/tools/clang
    get_llvm_project "http://llvm.org/git/compiler-rt.git"       ${MASON_BUILD_PATH}/projects/compiler-rt
    if [[ ${BUILD_AND_LINK_LIBCXX} == true ]]; then
        get_llvm_project "http://llvm.org/git/libcxx.git"            ${MASON_BUILD_PATH}/projects/libcxx
        get_llvm_project "http://llvm.org/git/libcxxabi.git"         ${MASON_BUILD_PATH}/projects/libcxxabi
        get_llvm_project "http://llvm.org/git/libunwind.git"         ${MASON_BUILD_PATH}/projects/libunwind
    fi
    get_llvm_project "http://llvm.org/git/lld.git"               ${MASON_BUILD_PATH}/tools/lld
    get_llvm_project "http://llvm.org/git/clang-tools-extra.git" ${MASON_BUILD_PATH}/tools/clang/tools/extra
    get_llvm_project "http://llvm.org/git/lldb.git"              ${MASON_BUILD_PATH}/tools/lldb
    get_llvm_project "https://github.com/include-what-you-use/include-what-you-use.git"  ${MASON_BUILD_PATH}/tools/clang/tools/include-what-you-use
}

mason_run "$@"
