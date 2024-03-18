#!/bin/sh
set -e

# we do not run `osrm-routed-js` tests on v12 since fastify doesn't support it
NODE_VERSION=$(node --version)
NODE_MAJOR_VERSION=$(echo $NODE_VERSION | cut -d. -f1)
if [[ "$NODE_MAJOR_VERSION" != "v12" ]]; then
    echo "Running osrm-routed-js tests"

    export OSRM_USE_ROUTED_JS=1
    node ./node_modules/cucumber/bin/cucumber.js features/ -p verify_routed_js
    node ./node_modules/cucumber/bin/cucumber.js features/ -p mld_routed_js
    unset OSRM_USE_ROUTED_JS
else 
    echo "Skipping osrm-routed-js tests on Node.js ${NODE_VERSION}"
fi

node ./node_modules/cucumber/bin/cucumber.js features/ -p verify
node ./node_modules/cucumber/bin/cucumber.js features/ -p verify -m mmap
node ./node_modules/cucumber/bin/cucumber.js features/ -p mld 
node ./node_modules/cucumber/bin/cucumber.js features/ -p mld -m mmap