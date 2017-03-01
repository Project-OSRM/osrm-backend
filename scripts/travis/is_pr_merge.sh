#!/bin/bash

set -eu pipefail

: '

This script is designed to detect if a gitsha represents a normal
push commit (to any branch) or whether it represents travis attempting
to merge between the origin and the upstream branch.

For more details see: https://docs.travis-ci.com/user/pull-requests

'

# Get the commit message via git log
# This should always be the exact text the developer provided
COMMIT_LOG=$(git log --format=%B --no-merges -n 1 | tr -d '\n')

# Get the commit message via git show
# If the gitsha represents a merge then this will
# look something like "Merge e3b1981 into 615d2a3"
# Otherwise it will be the same as the "git log" output
COMMIT_SHOW=$(git show -s --format=%B | tr -d '\n')

if [[ "${COMMIT_LOG}" != "${COMMIT_SHOW}" ]]; then
   echo true
fi
