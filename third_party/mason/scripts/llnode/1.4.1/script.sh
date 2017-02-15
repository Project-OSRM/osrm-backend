#!/usr/bin/env bash

MASON_NAME=llnode
MASON_VERSION=1.4.1
MASON_LIB_FILE=lib/llnode.${MASON_DYNLIB_SUFFIX}

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/nodejs/llnode/archive/v${MASON_VERSION}.tar.gz \
        b7ff83ff4686fdeb15517310b6e048c9c864794a

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    LLVM_VERSION=3.9.1
    ${MASON_DIR}/mason install llvm 3.9.1
    LLVM_PATH=$(${MASON_DIR}/mason prefix llvm 3.9.1)
}

function mason_compile {
    git clone --depth 1 https://chromium.googlesource.com/external/gyp.git tools/gyp
    # ../src/llv8.cc:256:43: error: expected ')'
     #snprintf(tmp, sizeof(tmp), " fn=0x%016" PRIx64, fn.raw());
    # need to define STDC macros since libc++ adheres to spec: http://en.cppreference.com/w/cpp/types/integer
    export CXXFLAGS="-stdlib=libc++ ${CXXFLAGS} -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS"
    export LDFLAGS="-stdlib=libc++ ${LDFLAGS}"
    # per the llvm package, on linux we statically link libc++ for full portability
    # while on osx we use the system libc++
    if [[ -f ${LLVM_PATH}/lib/libc++.a ]]; then
        export LDFLAGS="-Wl,--whole-archive ${LLVM_PATH}/lib/libc++.a ${LLVM_PATH}/lib/libc++abi.a ${LDFLAGS}"
    fi
    ./gyp_llnode -Dlldb_build_dir=${LLVM_PATH} -Dlldb_dir=${LLVM_PATH}
    make -C out/ -j${MASON_CONCURRENCY} V=1
    mkdir -p ${MASON_PREFIX}/lib
    if [[ $(uname -s) == 'Darwin' ]]; then
        cp ./out/Release/llnode* ${MASON_PREFIX}/lib/
    else
        cp ./out/Release/lib.target/llnode* ${MASON_PREFIX}/lib/
    fi
}

function mason_cflags {
    :
}

function mason_ldflags {
    echo -L${MASON_PREFIX} -llnode
}

function mason_static_libs {
    :
}

mason_run "$@"
