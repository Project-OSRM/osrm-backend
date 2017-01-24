#!/usr/bin/env bash

function assertEqual() {
    if [ "$1" == "$2" ]; then
        echo "ok - $1 ($3)"
    else
        echo "not ok - $1 != $2 ($3)"
        CODE=1
    fi
}
