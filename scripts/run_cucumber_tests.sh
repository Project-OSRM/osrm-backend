#!/bin/sh
set -e

export OSRM_USE_ROUTED_JS=1
node ./node_modules/cucumber/bin/cucumber.js features/ -p verify_routed_js
# node ./node_modules/cucumber/bin/cucumber.js features/ -p verify_routed_js -m mmap
# node ./node_modules/cucumber/bin/cucumber.js features/ -p mld_routed_js 
# node ./node_modules/cucumber/bin/cucumber.js features/ -p mld_routed_js -m mmap
unset OSRM_USE_ROUTED_JS

# node ./node_modules/cucumber/bin/cucumber.js features/ -p verify
# node ./node_modules/cucumber/bin/cucumber.js features/ -p verify -m mmap
# node ./node_modules/cucumber/bin/cucumber.js features/ -p mld 
# node ./node_modules/cucumber/bin/cucumber.js features/ -p mld -m mmap