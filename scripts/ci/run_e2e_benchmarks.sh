#!/bin/bash
set -e -o pipefail

DATASET=berlin
GPS_TRACES="${DATASET}_gps_traces.csv.gz"
LOG_DIR=test/logs
SCRIPTS_DIR=scripts/ci

source build/osrm-run-env.sh

function summary {
    if [ -n "$GITHUB_STEP_SUMMARY" ]; then
        printf '%b' "$1" >> $GITHUB_STEP_SUMMARY
    fi
}

summary "### e2e benchmarks\n"
summary "Mean time for answering 100 randomly generated requests, in ms.\n"
summary "| Algorithm | Route | Nearest | Trip | Table | Match |\n"
summary "| --------- | -----:| -------:| ----:| -----:| -----:|\n"

for ALGORITHM in ch mld; do
    "$OSRM_BUILD_DIR/osrm-routed" -a $ALGORITHM "$OSRM_TEST_DATA_DIR/$ALGORITHM/$DATASET.osrm" 2>&1 > /dev/null &
    OSRM_ROUTED_PID=$!

    if ! curl --retry-delay 1 --retry 30 --retry-all-errors \
            "http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true" > /dev/null 2>&1; then
        echo "osrm-routed failed to start for algorithm $ALGORITHM"
        kill -9 $OSRM_ROUTED_PID || true
        continue
    fi

    summary "| ${ALGORITHM}"

    for METHOD in route nearest trip table match; do
        echo "Running e2e benchmark for $METHOD $ALGORITHM"
        TIME=`python "$SCRIPTS_DIR/e2e_benchmark_simplified.py" --method $METHOD --gps_traces "$OSRM_TEST_DATA_DIR/$GPS_TRACES"`
        echo $TIME
        echo $TIME > "$LOG_DIR/e2e_${METHOD}_${ALGORITHM}.bench"
        summary " | $TIME"
    done

    summary " |\n"
    kill -9 $OSRM_ROUTED_PID || true
done
