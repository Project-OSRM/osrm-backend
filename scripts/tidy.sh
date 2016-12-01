#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Runs the Clang Tidy Tool in parallel on the code base.
# Requires a compilation database in the build directory.

# This works on both OSX and Linux, it's a POSIX thingy
NPROC=$(getconf _NPROCESSORS_ONLN)


find src include unit_tests -type f -name '*.hpp' -o -name '*.cpp' -print0 \
  | xargs \
      -0 \
      -I{} \
      -n 1 \
      ./clang+llvm-3.9.0-x86_64-apple-darwin/bin/clang-tidy \
        -p build \
        -header-filter='.*' \
      {}
