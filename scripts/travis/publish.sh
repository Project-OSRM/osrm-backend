#!/bin/bash

set -eu
set -o pipefail

# should be set for debug builds
export NPM_FLAGS=${NPM_FLAGS:-}

echo "node version is:"
which node
node -v

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal ${NPM_FLAGS}

# enforce that binary has proper ORIGIN flags so that
# it can portably find libtbb.so in the same directory
if [[ $(uname -s) == 'Linux' ]]; then
    readelf -d ./lib/binding/node-osrm.node > readelf-output.txt
    if grep -q 'Flags: ORIGIN' readelf-output.txt; then
        echo "Found ORIGIN flag in readelf output"
        cat readelf-output.txt
    else
        echo "*** Error: Could not found ORIGIN flag in readelf output"
        cat readelf-output.txt
        exit 1
    fi
fi

echo "determining publishing status..."

if [[ $(./scripts/travis/is_pr_merge.sh) ]]; then
    echo "Skipping publishing because this is a PR merge commit"
else
    echo "This is a push commit, continuing to package..."
    ./node_modules/.bin/node-pre-gyp package ${NPM_FLAGS}

    export COMMIT_MESSAGE=$(git log --format=%B --no-merges | head -n 1 | tr -d '\n')
    echo "Commit message: ${COMMIT_MESSAGE}"

    if [[ ${COMMIT_MESSAGE} =~ "[publish binary]" ]]; then
        echo "Publishing"
        ./node_modules/.bin/node-pre-gyp publish ${NPM_FLAGS}
    elif [[ ${COMMIT_MESSAGE} =~ "[republish binary]" ]]; then
        echo "*** Error: Republishing is disallowed for this repository"
        exit 1
        #./node_modules/.bin/node-pre-gyp unpublish publish ${NPM_FLAGS}
    else
        echo "Skipping publishing"
    fi;
fi
