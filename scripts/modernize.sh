#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Runs the Clang Modernizer in parallel on the code base.
# Requires a compilation database in the build directory.

find src include unit_tests -type f -name '*.hpp' -o -name '*.cpp' \
  | xargs \
      -I{} \
      -P $(nproc) \
      clang-modernize \
        -p build \
        -final-syntax-check \
        -format \
        -style=file \
        -summary \
        -for-compilers=clang-3.4,gcc-4.8 \
        -include . \
        -exclude third_party \
      {}
