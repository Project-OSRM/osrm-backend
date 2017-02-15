#!/usr/bin/env bash

MASON_NAME=llvm
MASON_LIB_FILE=bin/clang

. ${MASON_DIR}/mason.sh

export MASON_BASE_VERSION=${MASON_BASE_VERSION:-${MASON_VERSION}}
export MAJOR_MINOR=$(echo ${MASON_BASE_VERSION} | cut -d '.' -f1-2)

if [[ $(uname -s) == 'Darwin' ]]; then
    export BUILD_AND_LINK_LIBCXX=false
    # avoids this kind of problem with include-what-you-use
    # because iwyu hardcodes at https://github.com/include-what-you-use/include-what-you-use/blob/da5c9b17fec571e6b2bbca29145463d7eaa3582e/iwyu_driver.cc#L219
    : '
    /Library/Developer/CommandLineTools/usr/include/c++/v1/cstdlib:167:44: error: declaration conflicts with target of using declaration already in scope
    inline _LIBCPP_INLINE_VISIBILITY long      abs(     long __x) _NOEXCEPT {return  labs(__x);}
                                               ^
    /Users/dane/.mason/mason_packages/osx-x86_64/llvm/3.9.0/bin/../include/c++/v1/stdlib.h:115:44: note: target of using declaration
    inline _LIBCPP_INLINE_VISIBILITY long      abs(     long __x) _NOEXCEPT {return  labs(__x);}
    '
else
    export BUILD_AND_LINK_LIBCXX=${BUILD_AND_LINK_LIBCXX:-true}
fi

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
        mkdir -p ./checkout
        rm -rf ./checkout/*
        tar xf ${local_file_or_checkout} --strip-components=1 --directory=./checkout
        mkdir -p ${TO_DIR}
        mv checkout/* ${TO_DIR}/
    fi
}

# Note: override this function to set custom hash
function setup_release() {
    get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/llvm-${MASON_BASE_VERSION}.src.tar.xz"              ${MASON_BUILD_PATH}/
    get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/cfe-${MASON_BASE_VERSION}.src.tar.xz"               ${MASON_BUILD_PATH}/tools/clang
    get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/compiler-rt-${MASON_BASE_VERSION}.src.tar.xz"       ${MASON_BUILD_PATH}/projects/compiler-rt
    if [[ ${BUILD_AND_LINK_LIBCXX} == true ]]; then
        get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/libcxx-${MASON_BASE_VERSION}.src.tar.xz"        ${MASON_BUILD_PATH}/projects/libcxx
        get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/libcxxabi-${MASON_BASE_VERSION}.src.tar.xz"     ${MASON_BUILD_PATH}/projects/libcxxabi
        get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/libunwind-${MASON_BASE_VERSION}.src.tar.xz"     ${MASON_BUILD_PATH}/projects/libunwind
    fi
    get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/lld-${MASON_BASE_VERSION}.src.tar.xz"               ${MASON_BUILD_PATH}/tools/lld
    get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/clang-tools-extra-${MASON_BASE_VERSION}.src.tar.xz" ${MASON_BUILD_PATH}/tools/clang/tools/extra
    get_llvm_project "http://llvm.org/releases/${MASON_BASE_VERSION}/lldb-${MASON_BASE_VERSION}.src.tar.xz"              ${MASON_BUILD_PATH}/tools/lldb
    get_llvm_project "https://github.com/include-what-you-use/include-what-you-use/archive/clang_${MAJOR_MINOR}.tar.gz" ${MASON_BUILD_PATH}/tools/clang/tools/include-what-you-use
}

function mason_load_source {
    mkdir -p "${MASON_ROOT}/.cache"
    cd "${MASON_ROOT}/.cache"
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/llvm-${MASON_BASE_VERSION}
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

    # remove /usr/local/include from default paths (targeting linux)
    # because we want users to have to explictly link things in /usr/local to avoid conflicts
    # between mason and homebrew or source installs
    perl -i -p -e "s/AddPath\(\"\/usr\/local\/include\"\, System\, false\)\;//g;" tools/clang/lib/Frontend/InitHeaderSearch.cpp

    if [[ ${MAJOR_MINOR} == "3.8" ]]; then
        # workaround https://llvm.org/bugs/show_bug.cgi?id=25565
        perl -i -p -e "s/set\(codegen_deps intrinsics_gen\)/set\(codegen_deps intrinsics_gen attributes_inc\)/g;" lib/CodeGen/CMakeLists.txt

        # note: LIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON is only needed with llvm < 3.9.0 to avoid libcxx(abi) build breaking when only a static libc++ exists
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON"
    fi

    if [[ -d tools/clang/tools/include-what-you-use ]]; then
        echo  'add_subdirectory(include-what-you-use)' >> tools/clang/tools/CMakeLists.txt
    fi

    mkdir -p ./build
    cd ./build
    CMAKE_EXTRA_ARGS=""
    if [[ $(uname -s) == 'Darwin' ]]; then
        : '
        Note: C_INCLUDE_DIRS and DEFAULT_SYSROOT are critical options to understand to ensure C and C++ headers are predictably found.

        The way things work in clang++ on OS X (inside http://clang.llvm.org/doxygen/InitHeaderSearch_8cpp.html) is:

           - The `:` separated `C_INCLUDE_DIRS` are added to the include paths
           - If `C_INCLUDE_DIRS` is present `InitHeaderSearch::AddDefaultCIncludePaths` returns early
             - Without that early return `/usr/include` would be added by default on OS X
           - If `-isysroot` is passed then absolute `C_INCLUDE_DIRS` are appended to the sysroot
             - So if sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/ and
               C_INCLUDE_DIRS=/usr/include the actual path searched would be:
               /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
           - Relative `C_INCLUDE_DIRS` seem pointless because they are not appended to the sysroot and so will not be portable
           - clang++ finds C++ headers relative to itself at https://github.com/llvm-mirror/clang/blob/master/lib/Frontend/InitHeaderSearch.cpp#L469-L470
           - So, given on OS X we want to use the XCode/Apple provided libc++ and c++ headers we symlink the relative location to /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++
             - The alternative would be to symlink to the command line tools location (/Library/Developer/CommandLineTools/usr/include/c++/v1/)

        Another viable sysroot would be the command line tools at /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk

        Generally each SDK/Platform version has its own C headers inside SDK_PATH/usr/include while all platforms share the C++ headers which
        are at /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/

        NOTE: show search paths with: `clang -x c -v -E /dev/null` || `cpp -v` && `clang -Xlinker -v`
        '
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DC_INCLUDE_DIRS=/usr/include"
        # setting the default sysroot to an explicit SDK avoids clang++ adding `/usr/local/include` to the paths by default at https://github.com/llvm-mirror/clang/blob/91d69c3c9c62946245a0fe6526d5ec226dfe7408/lib/Frontend/InitHeaderSearch.cpp#L226
        # because that value will be appended to the sysroot, not exist, and then get thrown out. If the sysroot were / then it would be added
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DDEFAULT_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCLANG_DEFAULT_CXX_STDLIB=libc++"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11"
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLLVM_CREATE_XCODE_TOOLCHAIN=ON -DLLVM_EXTERNALIZE_DEBUGINFO=ON"
    fi
    if [[ $(uname -s) == 'Linux' ]]; then
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLLVM_BINUTILS_INCDIR=${LLVM_BINUTILS_INCDIR}"
        if [[ ${MAJOR_MINOR} == "3.8" ]] && [[ ${BUILD_AND_LINK_LIBCXX} == true ]]; then
            # note: LIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON is only needed with llvm < 3.9.0 to avoid libcxx(abi) build breaking when only a static libc++ exists
            CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON"
        fi
    fi

    # we strip this since we don't care about older os x for this package
    if [[ $(uname -s) == 'Darwin' ]]; then
        export CXXFLAGS="${CXXFLAGS//-mmacosx-version-min=10.8}"
        export LDFLAGS="${LDFLAGS//-mmacosx-version-min=10.8}"
    fi

    # llvm may request c++14 instead so let's not force c++11
    export CXXFLAGS="${CXXFLAGS//-std=c++11}"

    if [[ ${BUILD_AND_LINK_LIBCXX} == true ]]; then
        # we link to libc++ even on linux to avoid runtime dependency on libstdc++:
        # https://github.com/mapbox/mason/issues/252
        export CXXFLAGS="-stdlib=libc++ ${CXXFLAGS}"
        export LDFLAGS="-stdlib=libc++ ${LDFLAGS}"

        if [[ $(uname -s) == 'Linux' ]]; then
            export LDFLAGS="${LDFLAGS} -Wl,--start-group -L$(pwd)/lib -lc++ -lc++abi -pthread -lc -lgcc_s"
        fi
    fi

    # on linux the default is to link programs compiled by clang++ to libstdc++ and below we make that explicit.
    if [[ $(uname -s) == 'Linux' ]]; then
        export CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCLANG_DEFAULT_CXX_STDLIB=libstdc++"
    fi

    # TODO: test this
    #-DLLVM_ENABLE_LTO=ON \

    # TODO: try rtlib=compiler-rt on linux
    # https://blogs.gentoo.org/gsoc2016-native-clang/2016/05/31/build-gnu-free-executables-with-clang/

    if [[ ${BUILD_AND_LINK_LIBCXX} == true ]]; then
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DLIBCXX_ENABLE_ASSERTIONS=OFF -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXXABI_ENABLE_SHARED=OFF -DLIBCXXABI_USE_LLVM_UNWINDER=ON -DLIBUNWIND_ENABLE_SHARED=OFF"
    fi

    ${MASON_CMAKE}/bin/cmake ../ -G Ninja -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
     -DCMAKE_BUILD_TYPE=Release \
     -DLLVM_INCLUDE_DOCS=OFF \
     -DLLVM_TARGETS_TO_BUILD="X86" \
     -DCMAKE_CXX_COMPILER_LAUNCHER="${MASON_CCACHE}/bin/ccache" \
     -DCMAKE_CXX_COMPILER="$CXX" \
     -DCMAKE_C_COMPILER="$CC" \
     -DLLVM_ENABLE_ASSERTIONS=OFF \
     -DCLANG_VENDOR="mapbox/mason" \
     -DCLANG_REPOSITORY_STRING="https://github.com/mapbox/mason" \
     -DCLANG_VENDOR_UTI="org.mapbox.llvm" \
     -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
     -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
     -DLLDB_DISABLE_PYTHON=1 -DLLDB_DISABLE_CURSES=1 -DLLDB_DISABLE_LIBEDIT=1 -DLLVM_ENABLE_TERMINFO=0 \
     -DCMAKE_MAKE_PROGRAM=${MASON_NINJA}/bin/ninja \
     ${CMAKE_EXTRA_ARGS}

    if [[ ${BUILD_AND_LINK_LIBCXX} == true ]]; then
        ${MASON_NINJA}/bin/ninja unwind -j${MASON_CONCURRENCY}

        # make libc++ and libc++abi first
        ${MASON_NINJA}/bin/ninja cxx -j${MASON_CONCURRENCY}

        ${MASON_NINJA}/bin/ninja lldb -j${MASON_CONCURRENCY}
    fi

    # then make everything else
    ${MASON_NINJA}/bin/ninja -j${MASON_CONCURRENCY}

    # install it all
    ${MASON_NINJA}/bin/ninja install

    if [[ $(uname -s) == 'Darwin' ]]; then
        # https://reviews.llvm.org/D13605
        ${MASON_NINJA}/bin/ninja install-xcode-toolchain -j${MASON_CONCURRENCY}
    fi

    # install the asan_symbolizer.py tool
    cp -a ../projects/compiler-rt/lib/asan/scripts/asan_symbolize.py ${MASON_PREFIX}/bin/

    # set up symlinks to match what llvm.org binaries provide
    cd ${MASON_PREFIX}/bin/
    ln -s "clang++" "clang++-${MAJOR_MINOR}"
    ln -s "asan_symbolize.py" "asan_symbolize"

    # symlink so that we use the system libc++ headers on osx
    if [[ $(uname -s) == 'Darwin' ]]; then
        mkdir -p ${MASON_PREFIX}/include
        cd ${MASON_PREFIX}/include
        # note: passing -nostdinc++ will result in this local path being ignored
        ln -s /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++ c++
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
