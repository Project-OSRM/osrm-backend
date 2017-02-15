#!/usr/bin/env bash

CODE=0

source $(dirname $0)/assert.sh

VAL=$(./mason env MASON_DIR)
assertEqual "$?" "0" "able to run ./mason env MASON_DIR"
if [[ ${MASON_DIR:-unset} != "unset" ]]; then
    assertEqual "$MASON_DIR" "$VAL" "got correct result of ./mason env MASON_DIR"
else
    assertEqual "$(pwd)" "$VAL" "got correct result of ./mason env MASON_DIR"
fi

VAL=$(./mason --version)
assertEqual "$?" "0" "able to run ./mason --version"

exit $CODE