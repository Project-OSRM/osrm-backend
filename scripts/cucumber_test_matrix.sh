#!/usr/bin/env bash

set -o errexit
set -o pipefail

loadmethods=(mmap directly datastore)
algorithms=(ch mld)
base=home

function summary {
    if [ -n "$GITHUB_STEP_SUMMARY" ]; then
        printf '%b' "$1" >> $GITHUB_STEP_SUMMARY
    fi
}

if [ -n "$GITHUB_STEP_SUMMARY" ]; then
  base=github
  set +o errexit
  summary "### Cucumber Test Summary\n"
  summary "|Algorithm|Load Method|Passed|Skipped|Failed|Elapsed (s)|\n"
  summary "|:------- |:--------- | ----:| -----:| ----:| ---------:|\n"
fi

for algorithm in "${algorithms[@]}"
do
  export algorithm
  for loadmethod in "${loadmethods[@]}"
  do
    export loadmethod
    summary "| $algorithm | $loadmethod "
    set -x
    npx cucumber-js -p $base -p $algorithm -p $loadmethod $@
    { set +x; } 2>/dev/null
  done
done
