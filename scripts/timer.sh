#!/bin/bash

TIMINGS_FILE=/tmp/osrm.timings
NAME=$1
CMD=${@:2}
START=$(date "+%s.%N")
if [[ $(uname -s) == 'Darwin' ]] && [[ ${OSRM_SHARED_LIBRARY_PATH:-false} != false ]]; then
    /bin/bash -c "DYLD_LIBRARY_PATH=${OSRM_SHARED_LIBRARY_PATH} $CMD"
else
    /bin/bash -c "$CMD"
fi

END=$(date "+%s.%N")
TIME="$(echo "$END - $START" | bc)s"
NEW_ENTRY="$NAME\t$TIME\t$(date -Iseconds)"

echo -e "$NEW_ENTRY" >> $TIMINGS_FILE
