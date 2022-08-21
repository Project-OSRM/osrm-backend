#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Runs the Clang Formatter in parallel on the code base.
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

# Get CPU count
OS=$(uname)
NPROC=1
if [[ $OS = "Linux" ]] ; then
    NPROC=$(nproc)
elif [[ ${OS} = "Darwin" ]] ; then
    NPROC=$(sysctl -n hw.physicalcpu)
fi

# Discover clang-format
if type clang-format-10.0.0 2> /dev/null ; then
    CLANG_FORMAT=clang-format-10.0.0
elif type clang-format 2> /dev/null ; then
    # Clang format found, but need to check version
    CLANG_FORMAT=clang-format
    V=$(clang-format --version)
    if [[ $V != *10.0* ]] ; then
        echo "clang-format is not 10.0 (returned ${V})"
        #exit 1
    fi
else
    echo "No appropriate clang-format found (expected clang-format-10.0.0, or clang-format)"
    exit 1
fi

find src include unit_tests example -type f -name '*.hpp' -o -name '*.cpp' \
  | xargs -I{} -P ${NPROC} ${CLANG_FORMAT} -i -style=file {}
