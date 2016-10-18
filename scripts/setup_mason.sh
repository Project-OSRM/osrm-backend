#!/bin/bash

set -eu
set -o pipefail

# we pin the mason version to avoid changes in mason breaking builds
MASON_VERSION="ecee686"

function main() {
    if [[ ! -d ./.mason ]]; then
        echo "Cloning mason at https://github.com/mapbox/mason/commit/${MASON_VERSION}"
        git clone https://github.com/mapbox/mason.git ./.mason
        (cd ./.mason && git checkout ${MASON_VERSION})
    else
        echo "Updating to mason at https://github.com/mapbox/mason/commit/${MASON_VERSION}"
        (cd ./.mason && git fetch && git pull || true && git checkout ${MASON_VERSION})
    fi
}

main

set +eu
set +o pipefail