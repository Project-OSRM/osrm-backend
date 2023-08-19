#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

loadmethods=(datastore mmap directly)
profiles=(ch mld)

for profile in "${profiles[@]}"
do
  for loadmethod in "${loadmethods[@]}"
  do
    set -x
    node ./node_modules/cucumber/bin/cucumber.js features/ -p $profile -m $loadmethod
    { set +x; } 2>/dev/null
  done
done
