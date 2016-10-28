#!/usr/bin/env bash

MASON_NAME=clang
MASON_VERSION=3.8.0
MASON_LIB_FILE=bin/clang

. ${MASON_DIR}/mason.sh


# options
ENABLE_LLDB=false

function curl_get() {
    if [ ! -f $(basename ${1}) ] ; then
        mason_step "Downloading $1 to $(pwd)/$(basename ${1})"
        curl --retry 3 -f -L -O "$1"
    else
        echo "already downloaded $1 to $(pwd)/$(basename ${1})"
    fi
}

function setup_release() {
    LLVM_RELEASE=$1
    BUILD_PATH=$2
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/llvm-${LLVM_RELEASE}.src.tar.xz"
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/cfe-${LLVM_RELEASE}.src.tar.xz"
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/compiler-rt-${LLVM_RELEASE}.src.tar.xz"
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/libcxx-${LLVM_RELEASE}.src.tar.xz"
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/libcxxabi-${LLVM_RELEASE}.src.tar.xz"
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/libunwind-${LLVM_RELEASE}.src.tar.xz"
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/lld-${LLVM_RELEASE}.src.tar.xz"
    if [[ ${ENABLE_LLDB} == true ]]; then
        curl_get "http://llvm.org/releases/${LLVM_RELEASE}/lldb-${LLVM_RELEASE}.src.tar.xz"
    fi
    #curl_get "http://llvm.org/releases/${LLVM_RELEASE}/openmp-${LLVM_RELEASE}.src.tar.xz"
    curl_get "http://llvm.org/releases/${LLVM_RELEASE}/clang-tools-extra-${LLVM_RELEASE}.src.tar.xz"
    for i in $(ls *.xz); do
        echo "unpacking $i"
        tar xf $i;
    done
    mv llvm-${LLVM_RELEASE}.src/* ${BUILD_PATH}/
    ls ${BUILD_PATH}/
    mv cfe-${LLVM_RELEASE}.src ${BUILD_PATH}/tools/clang
    mv compiler-rt-${LLVM_RELEASE}.src ${BUILD_PATH}/projects/compiler-rt
    mv libcxx-${LLVM_RELEASE}.src ${BUILD_PATH}/projects/libcxx
    mv libcxxabi-${LLVM_RELEASE}.src ${BUILD_PATH}/projects/libcxxabi
    mv libunwind-${LLVM_RELEASE}.src ${BUILD_PATH}/projects/libunwind
    mv lld-${LLVM_RELEASE}.src ${BUILD_PATH}/tools/lld
    if [[ ${ENABLE_LLDB} == true ]]; then
        mv lldb-${LLVM_RELEASE}.src ${BUILD_PATH}/tools/lldb
    fi
    #mv openmp-${LLVM_RELEASE}.src ${BUILD_PATH}/projects/openmp
    mv clang-tools-extra-${LLVM_RELEASE}.src ${BUILD_PATH}/tools/clang/tools/extra
    cd ../
}


function mason_load_source {
    mkdir -p "${MASON_ROOT}/.cache"
    cd "${MASON_ROOT}/.cache"
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/llvm-${MASON_VERSION}
    if [[ -d ${MASON_BUILD_PATH}/ ]]; then
        rm -rf ${MASON_BUILD_PATH}/
    fi
    mkdir -p ${MASON_BUILD_PATH}/
    setup_release ${MASON_VERSION} ${MASON_BUILD_PATH}
}

function mason_prepare_compile {
    ${MASON_DIR}/mason install ccache 3.2.4
    MASON_CCACHE=$(${MASON_DIR}/mason prefix ccache 3.2.4)
    ${MASON_DIR}/mason install clang 3.8.0
    MASON_CLANG=$(${MASON_DIR}/mason prefix clang 3.8.0)
    ${MASON_DIR}/mason install cmake 3.5.2
    MASON_CMAKE=$(${MASON_DIR}/mason prefix cmake 3.5.2)
    ${MASON_DIR}/mason install ninja 1.7.1
    MASON_NINJA=$(${MASON_DIR}/mason prefix ninja 1.7.1)
}

function mason_compile {
    export CXX="${MASON_CLANG}/bin/clang++"
    export CC="${MASON_CLANG}/bin/clang"
    mkdir -p ./build
    cd ./build
    CMAKE_EXTRA_ARGS=""
    ## TODO: CLANG_DEFAULT_CXX_STDLIB and CLANG_APPEND_VC_REV not available in clang-3.8 cmake files
    if [[ $(uname -s) == 'Darwin' ]]; then
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCLANG_DEFAULT_CXX_STDLIB=libc++"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DC_INCLUDE_DIRS=:/usr/include:/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/:/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DDEFAULT_SYSROOT=/"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11"
    fi
    CXXFLAGS="${CXXFLAGS//-mmacosx-version-min=10.8}"
    ${MASON_CMAKE}/bin/cmake ../ -G Ninja -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_CXX_COMPILER_LAUNCHER="${MASON_CCACHE}/bin/ccache" \
     -DCMAKE_CXX_COMPILER="$CXX" \
     -DCMAKE_C_COMPILER="$CC" \
     -DLLVM_ENABLE_ASSERTIONS=OFF \
     -DCLANG_VENDOR=mapbox/mason \
     -DCLANG_REPOSITORY_STRING=https://github.com/mapbox/mason \
     -DCLANG_VENDOR_UTI=org.mapbox.clang \
     -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
     -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
     -DLLVM_OPTIMIZED_TABLEGEN=ON \
     -DCMAKE_MAKE_PROGRAM=${MASON_NINJA}/bin/ninja \
     ${CMAKE_EXTRA_ARGS}
    ${MASON_NINJA}/bin/ninja -j${MASON_CONCURRENCY} -k5
    ${MASON_NINJA}/bin/ninja install -k5
    cd ${MASON_PREFIX}/bin/
    rm -f "clang++-3.8"
    ln -s "clang++" "clang++-3.8"
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

mason_run "$@"
