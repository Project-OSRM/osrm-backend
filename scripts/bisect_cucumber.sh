#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Automates bisecting cucumber tests in a portable way; usage:
#
#     git bisect start GOODGITSHA BADGITSHA
#     git bisect run /path/to/bisect_cucumber.sh
#
# XXX: store this file outside source control first, e.g. by copying it over
#      to /tmp, otherwise jumping through commits will change this script, too.


BUILD_DIR=build

cmake -E remove_directory $BUILD_DIR
cmake -E make_directory $BUILD_DIR
cmake -E chdir $BUILD_DIR cmake .. -DCMAKE_BUILD_TYPE=Release
cmake -E chdir $BUILD_DIR cmake --build .
cucumber -p verify


# notes on the return codes git bisect understands:
# - exit code 0 means okay
# - exit code 125 means skip this commit and try a commit nearby
# - every other exit code means bad
