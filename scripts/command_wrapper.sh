#!/bin/bash

set -eu
set -o pipefail

if [[ ${@:-false} == false ]]; then
    echo "please pass path to command you want to exec"
    exit 1
fi

echo $@

if [[ $(uname -s) == 'Darwin' ]] && [[ ${OSRM_SHARED_LIBRARY_PATH:-false} != false ]]; then
    DYLD_LIBRARY_PATH=${OSRM_SHARED_LIBRARY_PATH} $@
else
    $@
fi
