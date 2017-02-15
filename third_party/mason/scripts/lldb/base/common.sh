#!/usr/bin/env bash

function mason_build {
    ${MASON_DIR}/mason install llvm ${MASON_VERSION}
    CLANG_PREFIX=$(${MASON_DIR}/mason prefix llvm ${MASON_VERSION})
    # copy bin
    mkdir -p "${MASON_PREFIX}/bin"
    cp "${CLANG_PREFIX}/bin/${MASON_NAME}" "${MASON_PREFIX}/bin/"
    cp "${CLANG_PREFIX}/bin/lldb-server" "${MASON_PREFIX}/bin/"
    cp "${CLANG_PREFIX}/bin/lldb-argdumper" "${MASON_PREFIX}/bin/"
    # copy lib
    mkdir -p "${MASON_PREFIX}/lib"
    if [[ $(uname -s) == 'Darwin' ]]; then
        cp -r ${CLANG_PREFIX}/lib/liblldb*.dylib "${MASON_PREFIX}/lib/"
    else
        cp -r ${CLANG_PREFIX}/lib/liblldb*.so* "${MASON_PREFIX}/lib/"
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