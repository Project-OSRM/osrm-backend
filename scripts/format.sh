#!/usr/bin/env bash

# Runs the Clang Formatter in parallel on the code base.
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

set -eu -o pipefail

find src include unit_tests example -type f -name '*.hpp' -o -name '*.cpp' \
  | xargs -I{} -P $(nproc) clang-format -i -style=file {}


dirty=$(git ls-files --modified)

if [[ $dirty ]]; then
    echo "The following files do not adhere to the .clang-format style file:"
    echo $dirty
    exit 1
else
    exit 0
fi
