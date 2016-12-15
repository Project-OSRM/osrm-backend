#!/usr/bin/env bash

function mason_build {
    ${MASON_DIR}/mason install llvm ${MASON_VERSION}
    CLANG_PREFIX=$(${MASON_DIR}/mason prefix llvm ${MASON_VERSION})

    # copy bin
    mkdir -p "${MASON_PREFIX}/bin"
    cp "${CLANG_PREFIX}/bin/${MASON_NAME}" "${MASON_PREFIX}/bin/"

    # copy include/c++
    mkdir -p "${MASON_PREFIX}/include"
    # if custom libc++ was built
    if [[ $(uname -s) == 'Linux' ]]; then
        cp -r "${CLANG_PREFIX}/include/c++" "${MASON_PREFIX}/include/"
    fi
    # copy libs
    mkdir -p "${MASON_PREFIX}/lib"
    mkdir -p "${MASON_PREFIX}/lib/clang"
    cp -r ${CLANG_PREFIX}/lib/clang/${MASON_VERSION} "${MASON_PREFIX}/lib/clang/"
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