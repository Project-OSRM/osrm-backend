#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

TIMINGS_FILE=/tmp/osrm.timings
NAME=$1
CMD=${@:2}
START=$(date "+%s.%N")
/bin/bash -c "$CMD"
END=$(date "+%s.%N")
TIME="$(node -e "console.log($END - $START)")s"
NEW_ENTRY="$NAME\t$TIME"

echo -e "$NEW_ENTRY" >> $TIMINGS_FILE
