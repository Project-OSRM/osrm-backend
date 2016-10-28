#!/usr/bin/env bash

# dynamically determine the path to this package
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"

# dynamically take name of package from directory
MASON_NAME=$(basename $(dirname $HERE))
# dynamically take the version of the package from directory
MASON_VERSION=$(basename $HERE)
MASON_LIB_FILE=bin/${MASON_NAME}

. ${MASON_DIR}/mason.sh

function mason_build {
    ${MASON_DIR}/mason install clang ${MASON_VERSION}
    CLANG_PREFIX=$(${MASON_DIR}/mason prefix clang ${MASON_VERSION})
    mkdir -p "${MASON_PREFIX}/bin"
    cp ${CLANG_PREFIX}/bin/${MASON_NAME} "${MASON_PREFIX}/bin/"
}

mason_run "$@"
