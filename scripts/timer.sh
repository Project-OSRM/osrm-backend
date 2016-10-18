#!/bin/bash

TIMINGS_FILE=/tmp/osrm.timings
NAME=$1
CMD=${@:2}
START=$(date "+%s.%N")
/bin/bash -c "$CMD"
END=$(date "+%s.%N")
TIME="$(echo "$END - $START" | bc)s"
NEW_ENTRY="$NAME\t$TIME"

echo -e "$NEW_ENTRY" >> $TIMINGS_FILE
