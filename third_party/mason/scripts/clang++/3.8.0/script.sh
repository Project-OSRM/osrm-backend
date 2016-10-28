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
    # we need to copy the standard C++ headers because clang-tidy doesn't know
    # where to find them in the system and complains about missing information
    mkdir -p "${MASON_PREFIX}/include"
    mkdir -p "${MASON_PREFIX}/lib/clang/${MASON_VERSION}"
    cp ${CLANG_PREFIX}/bin/${MASON_NAME} "${MASON_PREFIX}/bin/"
    cp ${CLANG_PREFIX}/bin/clang "${MASON_PREFIX}/bin/"
    cp -R ${CLANG_PREFIX}/include/c++ "${MASON_PREFIX}/include/c++"
    cp -R ${CLANG_PREFIX}/lib/clang/${MASON_VERSION}/* "${MASON_PREFIX}/lib/clang/${MASON_VERSION}/"
    cd ${MASON_PREFIX}/bin/
    rm -f "clang++-3.8"
    ln -s "clang++" "clang++-3.8"
    rm -f "clang-3.8"
    ln -s "clang" "clang-3.8"
}

mason_run "$@"
