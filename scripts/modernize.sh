#!/usr/bin/env bash

# Runs the Clang Modernizer in parallel on the code base.
# Requires a compilation database in the build directory.

git ls-files '*.cpp' | xargs -I{} -P $(nproc) clang-modernize -p build -final-syntax-check -format -style=file -summary -for-compilers=clang-3.4,gcc-4.8 -include . -exclude third_party {}
