#!/usr/bin/env bash

set -o errexit
set -o pipefail

loadmethods=(mmap directly datastore)
algorithms=(ch mld)
base=home

if [ -n "$GITHUB_STEP_SUMMARY" ]; then
  base=github
  echo "### Cucumber Test Summary"                                 >> $GITHUB_STEP_SUMMARY
  echo ""                                                          >> $GITHUB_STEP_SUMMARY
  echo "|Algorithm|Load Method|Passed|Skipped|Failed|Elapsed (s)|" >> $GITHUB_STEP_SUMMARY
  echo "|:------- |:--------- | ----:| -----:| ----:| ---------:|" >> $GITHUB_STEP_SUMMARY
fi

for algorithm in "${algorithms[@]}"
do
  export algorithm
  for loadmethod in "${loadmethods[@]}"
  do
    export loadmethod
    if [ -n "$GITHUB_STEP_SUMMARY" ]; then
      echo -n "| $algorithm | $loadmethod " >> $GITHUB_STEP_SUMMARY
    fi
    set -x
    npx cucumber-js -p $base -p $algorithm -p $loadmethod $@
    { set +x; } 2>/dev/null
  done
done
