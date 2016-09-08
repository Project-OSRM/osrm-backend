#!/bin/bash

function now_ms() {
    if [[ "$(uname)" == Darwin ]] ; then
        if [[ ! -z $(which gdate) ]] ; then
            gdate "+%s.%N"
        else
            python -c 'import time; print time.time()'
        fi
    else
        date "+%s.%N"
    fi
}

TIMINGS_FILE=/tmp/osrm.timings
NAME=$1
CMD=${@:2}
START=$(now_ms)
/bin/bash -c "$CMD"
END=$(now_ms)
TIME="$(echo "$END - $START" | bc)s"
NEW_ENTRY="$NAME\t$TIME\t$(date +%FT%X)"

echo -e "$NEW_ENTRY" >> $TIMINGS_FILE
