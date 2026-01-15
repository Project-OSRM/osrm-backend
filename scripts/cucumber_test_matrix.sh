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
    OSRM_LOAD_METHOD=$loadmethod npx cucumber-js features/ -p $profile
    { set +x; } 2>/dev/null
  done
done
