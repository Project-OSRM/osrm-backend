#!/usr/bin/env bash

MASON_NAME=llvm
MASON_LIB_FILE=bin/clang

. ${MASON_DIR}/mason.sh

# we use this custom function rather than "mason_download" since we need to easily grab multiple packages
function get_llvm_project() {
    local URL=${1}
    local TO_DIR=${2}
    if [[ ${TO_DIR:-false} == false ]]; then
        mason_error "TO_DIR unset"
        exit 1
    fi
    local EXPECTED_HASH=${3:-false}
    local file_basename=$(basename ${URL})
    local local_file_or_checkout=$(pwd)/${file_basename}
    if [[ ${URL} =~ '.git' ]]; then
        if [ ! -d ${local_file_or_checkout} ] ; then
            mason_step "cloning ${URL} to ${local_file_or_checkout}"
            git clone --depth 1 ${URL} ${local_file_or_checkout}
        else
            mason_substep "already cloned ${URL}, pulling to update"
            (cd ${local_file_or_checkout} && git pull)
        fi
        mason_step "moving ${local_file_or_checkout} into place at ${TO_DIR}"
        cp -r ${local_file_or_checkout} ${TO_DIR}
    else
        if [ ! -f ${local_file_or_checkout} ] ; then
            mason_step "Downloading ${URL} to ${local_file_or_checkout}"
            curl --retry 3 -f -L -O "${URL}"
        else
            mason_substep "already downloaded $1 to ${local_file_or_checkout}"
        fi
        export OBJECT_HASH=$(git hash-object ${local_file_or_checkout})
        if [[ ${EXPECTED_HASH:-false} == false ]]; then
            mason_error "Warning: no expected hash provided by script.sh, actual was ${OBJECT_HASH}"
        else
            if [[ $3 != ${OBJECT_HASH} ]]; then
                mason_error "Error: hash mismatch ${EXPECTED_HASH} (expected) != ${OBJECT_HASH} (actual)"
                exit 1
            else
                mason_success "Success: hash matched: ${EXPECTED_HASH} (expected) == ${OBJECT_HASH} (actual)"
            fi
        fi
        mason_step "uncompressing ${local_file_or_checkout}"
        tar xf ${local_file_or_checkout}
        local uncompressed_dir=${file_basename/.tar.xz}
        mason_step "moving ${uncompressed_dir} into place at ${TO_DIR}"
        mv ${uncompressed_dir} ${TO_DIR}
    fi
}

# Note: override this function to set custom hash
function setup_release() {
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/llvm-${MASON_VERSION}.src.tar.xz"              ${MASON_BUILD_PATH}/                        
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/cfe-${MASON_VERSION}.src.tar.xz"               ${MASON_BUILD_PATH}/tools/clang             
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/compiler-rt-${MASON_VERSION}.src.tar.xz"       ${MASON_BUILD_PATH}/projects/compiler-rt    
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/libcxx-${MASON_VERSION}.src.tar.xz"            ${MASON_BUILD_PATH}/projects/libcxx         
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/libcxxabi-${MASON_VERSION}.src.tar.xz"         ${MASON_BUILD_PATH}/projects/libcxxabi      
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/libunwind-${MASON_VERSION}.src.tar.xz"         ${MASON_BUILD_PATH}/projects/libunwind      
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/lld-${MASON_VERSION}.src.tar.xz"               ${MASON_BUILD_PATH}/tools/lld               
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/clang-tools-extra-${MASON_VERSION}.src.tar.xz" ${MASON_BUILD_PATH}/tools/clang/tools/extra 
    get_llvm_project "http://llvm.org/releases/${MASON_VERSION}/lldb-${MASON_VERSION}.src.tar.xz"              ${MASON_BUILD_PATH}/tools/lldb
}

function mason_load_source {
    mkdir -p "${MASON_ROOT}/.cache"
    cd "${MASON_ROOT}/.cache"
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/llvm-${MASON_VERSION}
    mkdir -p "${MASON_ROOT}/.build"
    if [[ -d ${MASON_BUILD_PATH}/ ]]; then
        rm -rf ${MASON_BUILD_PATH}/
    fi
    # NOTE: this setup_release can be overridden per package to assert on different hash
    setup_release
}

function mason_prepare_compile {
    CCACHE_VERSION=3.3.1
    CMAKE_VERSION=3.6.2
    NINJA_VERSION=1.7.1

    ${MASON_DIR}/mason install ccache ${CCACHE_VERSION}
    MASON_CCACHE=$(${MASON_DIR}/mason prefix ccache ${CCACHE_VERSION})
    ${MASON_DIR}/mason install cmake ${CMAKE_VERSION}
    MASON_CMAKE=$(${MASON_DIR}/mason prefix cmake ${CMAKE_VERSION})
    ${MASON_DIR}/mason install ninja ${NINJA_VERSION}
    MASON_NINJA=$(${MASON_DIR}/mason prefix ninja ${NINJA_VERSION})

    if [[ $(uname -s) == 'Linux' ]]; then
        BINUTILS_VERSION=2.27
        ${MASON_DIR}/mason install binutils ${BINUTILS_VERSION}
        LLVM_BINUTILS_INCDIR=$(${MASON_DIR}/mason prefix binutils ${BINUTILS_VERSION})/include
    fi
}

function mason_compile {
    export CXX="${CXX:-clang++}"
    export CC="${CC:-clang}"
    # knock out lldb doc building, to remove doxygen dependency
    perl -i -p -e "s/add_subdirectory\(docs\)//g;" tools/lldb/CMakeLists.txt

    mkdir -p ./build
    cd ./build
    CMAKE_EXTRA_ARGS=""
    if [[ $(uname -s) == 'Darwin' ]]; then
        # This is a stable location for libc++ headers from the system
        SYSTEM_LIBCXX_HEADERS="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/"
        # This is the location of C headers on 10.11
        OSX_10_11_SDK_C_HEADERS="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/"
        # This is the location of C headers on >= 10.12 which is a symlink to the versioned SDK
        OSX_10_12_AND_GREATER_SDK_HEADERS="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/"
        # This allows this version of clang to find the headers from only a command line tools install (no xcode installed)
        # It is debatable whether this should be supported
        COMMAND_LINE_TOOLS_C_HEADERS="/usr/include"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DC_INCLUDE_DIRS=:${SYSTEM_LIBCXX_HEADERS}:${OSX_10_12_AND_GREATER_SDK_HEADERS}:${OSX_10_11_SDK_C_HEADERS}:${COMMAND_LINE_TOOLS_C_HEADERS}"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCLANG_DEFAULT_CXX_STDLIB=libc++"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DDEFAULT_SYSROOT=/"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLLVM_CREATE_XCODE_TOOLCHAIN=ON -DLLVM_EXTERNALIZE_DEBUGINFO=ON"
    fi
    MAJOR_MINOR=$(echo $MASON_VERSION | cut -d '.' -f1-2)

    if [[ $(uname -s) == 'Linux' ]]; then
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLLVM_BINUTILS_INCDIR=${LLVM_BINUTILS_INCDIR}"
        if [[ ${MAJOR_MINOR} == "3.8" ]]; then
            # note: LIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON is only needed with llvm < 3.9.0 to avoid libcxx(abi) build breaking when only a static libc++ exists
            CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON"
        fi
    fi

    # we link to libc++ even on linux to avoid runtime dependency on libstdc++:
    # https://github.com/mapbox/mason/issues/252
    export CXXFLAGS="-stdlib=libc++ ${CXXFLAGS//-mmacosx-version-min=10.8}"
    export LDFLAGS="-stdlib=libc++ ${LDFLAGS//-mmacosx-version-min=10.8}"
    if [[ $(uname -s) == 'Linux' ]]; then
        export LDFLAGS="${LDFLAGS} -Wl,--start-group -L$(pwd)/lib -lc++ -lc++abi -pthread -lc -lgcc_s"

    fi
    # llvm may request c++14 instead so let's not force c++11
    export CXXFLAGS="${CXXFLAGS//-std=c++11}"

    # TODO: test this
    #-DLLVM_ENABLE_LTO=ON \


    ${MASON_CMAKE}/bin/cmake ../ -G Ninja -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
     -DCMAKE_BUILD_TYPE=Release \
     -DLLVM_TARGETS_TO_BUILD="ARM;X86;AArch64" \
     -DCMAKE_CXX_COMPILER_LAUNCHER="${MASON_CCACHE}/bin/ccache" \
     -DCMAKE_CXX_COMPILER="$CXX" \
     -DCMAKE_C_COMPILER="$CC" \
     -DLIBCXX_ENABLE_ASSERTIONS=OFF \
     -DLIBCXX_ENABLE_SHARED=OFF \
     -DLIBCXXABI_ENABLE_SHARED=OFF \
     -DLIBUNWIND_ENABLE_SHARED=OFF \
     -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
     -DLLVM_ENABLE_ASSERTIONS=OFF \
     -DCLANG_VENDOR="mapbox/mason" \
     -DCLANG_REPOSITORY_STRING="https://github.com/mapbox/mason" \
     -DCLANG_VENDOR_UTI="org.mapbox.llvm" \
     -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
     -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
     -DLLDB_DISABLE_PYTHON=1 -DLLDB_DISABLE_CURSES=1 -DLLDB_DISABLE_LIBEDIT=1 -DLLVM_ENABLE_TERMINFO=0 \
     -DCMAKE_MAKE_PROGRAM=${MASON_NINJA}/bin/ninja \
     ${CMAKE_EXTRA_ARGS}

    ${MASON_NINJA}/bin/ninja unwind -j${MASON_CONCURRENCY}

    # make libc++ and libc++abi first
    # this ensures that the LD_LIBRARY_PATH above will be valid
    # and that clang++ on linux will be able to link itself to
    # this same instance of libc++
    ${MASON_NINJA}/bin/ninja cxx -j${MASON_CONCURRENCY}

    ${MASON_NINJA}/bin/ninja lldb -j${MASON_CONCURRENCY}
    # no move the host compilers libc++ and libc++abi shared libs out of the way
    if [[ ${CXX_BOOTSTRAP:-false} != false ]]; then
        mkdir -p /tmp/backup_shlibs
        mv $(dirname $(dirname $CXX))/lib/*c++*so /tmp/backup_shlibs/
    fi
    # then make everything else
    ${MASON_NINJA}/bin/ninja -j${MASON_CONCURRENCY}
    # install it all
    ${MASON_NINJA}/bin/ninja install
    # set up symlinks for clang++ to match what llvm.org binaries provide
    cd ${MASON_PREFIX}/bin/
    ln -s "clang++" "clang++-${MAJOR_MINOR}"
    # restore host compilers sharedlibs
    if [[ ${CXX_BOOTSTRAP:-false} != false ]]; then
        cp -r /tmp/backup_shlibs/* $(dirname $(dirname $CXX))/lib/
    fi
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
