#!/usr/bin/env bash

MASON_NAME=clang-tidy
MASON_VERSION=3.8
MASON_LIB_FILE=bin/clang-tidy

. ${MASON_DIR}/mason.sh

function mason_build {
    mkdir -p "${MASON_PREFIX}/bin"
    cd "${MASON_PREFIX}"

    URL="https://mason-binaries.s3.amazonaws.com/prebuilt/${MASON_PLATFORM}-$(uname -m)/clang-tidy-${MASON_VERSION}"
    curl "${URL}" -o "${MASON_LIB_FILE}"
}

mason_run "$@"
