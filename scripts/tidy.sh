#!/usr/bin/env bash

# Runs the Clang Tidy Tool in parallel on the code base.
# Requires a compilation database in the build directory.

git ls-files '*.cpp' | grep -v third_party | xargs -I{} -P $(nproc) clang-tidy -p build -header-filter='.*' {}
