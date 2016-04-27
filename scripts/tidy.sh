#!/usr/bin/env bash

# Runs the Clang Tidy Tool in parallel on the code base.
# Requires a compilation database in the build directory.


find src include unit_tests -type f -name '*.hpp' -o -name '*.cpp' \
  | xargs \
      -I{} \
      -P $(nproc) \
      clang-tidy \
        -p build \
        -header-filter='.*' \
      {}
