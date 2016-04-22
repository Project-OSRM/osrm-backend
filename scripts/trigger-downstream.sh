#!/bin/bash

set -eu
set -o pipefail

TRAVIS_COMMIT=${TRAVIS_COMMIT:-$(git log --format=%H --no-merges -n 1 | tr -d '\n')}

trigger_downstream() {
    curl -s -X POST \
      -H "Content-Type: application/json" \
      -H "Accept: application/json" \
      -H "Travis-API-Version: 3" \
      -H "Authorization: token ${TRAVIS_TOKEN}" \
      -d "$(python -c "import json;print open('./scripts/trigger-config.json').read().replace('<TRAVIS_COMMIT>','$TRAVIS_COMMIT')")" \
      https://api.travis-ci.org/repo/Project-OSRM%2Fnode-osrm/requests
}

trigger_downstream
