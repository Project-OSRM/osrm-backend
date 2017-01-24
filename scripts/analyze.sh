#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Runs the Static Analyzer on the code base.
# This is a wrapper intended to be used with like this:
# 1/ analyze cmake ..
# 2/ analyze cmake --build .


exec scan-build -analyze-headers -no-failure-reports --keep-going --status-bugs \
  -enable-checker alpha.core.BoolAssignment \
  -enable-checker alpha.core.IdenticalExpr \
  -enable-checker alpha.core.TestAfterDivZero \
  -enable-checker alpha.deadcode.UnreachableCode \
  -enable-checker alpha.security.ArrayBoundV2 \
  -enable-checker alpha.security.MallocOverflow \
  -enable-checker alpha.security.ReturnPtrRange \
  -enable-checker security.FloatLoopCounter \
  -enable-checker security.insecureAPI.rand \
  -enable-checker security.insecureAPI.strcpy \
  "${@}"
