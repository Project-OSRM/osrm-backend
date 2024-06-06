#!/bin/bash
set -eou pipefail

function run_benchmarks_for_folder {
    echo "Running benchmarks for $1"

    FOLDER=$1
    RESULTS_FOLDER=$2

    mkdir -p $RESULTS_FOLDER

    BENCHMARKS_FOLDER="$FOLDER/build/src/benchmarks"

    ./$BENCHMARKS_FOLDER/match-bench "./$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/match_mld.bench"
    ./$BENCHMARKS_FOLDER/match-bench "./$FOLDER/test/data/ch/monaco.osrm" ch > "$RESULTS_FOLDER/match_ch.bench"
    ./$BENCHMARKS_FOLDER/route-bench "./$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/route_mld.bench"
    ./$BENCHMARKS_FOLDER/route-bench "./$FOLDER/test/data/ch/monaco.osrm" ch > "$RESULTS_FOLDER/route_ch.bench"
    ./$BENCHMARKS_FOLDER/alias-bench > "$RESULTS_FOLDER/alias.bench"
    ./$BENCHMARKS_FOLDER/json-render-bench  "./$FOLDER/src/benchmarks/portugal_to_korea.json" > "$RESULTS_FOLDER/json-render.bench"
    ./$BENCHMARKS_FOLDER/packedvector-bench > "$RESULTS_FOLDER/packedvector.bench"
    ./$BENCHMARKS_FOLDER/rtree-bench "./$FOLDER/test/data/monaco.osrm.ramIndex" "./$FOLDER/test/data/monaco.osrm.fileIndex" "./$FOLDER/test/data/monaco.osrm.nbg_nodes" > "$RESULTS_FOLDER/rtree.bench"

    BINARIES_FOLDER="$FOLDER/build"

    cp ~/data.osm.pbf $FOLDER
    /usr/bin/time -f "Time: %E Peak RAM: %M Kb" $BINARIES_FOLDER/osrm-extract -p $FOLDER/profiles/car.lua $FOLDER/data.osm.pbf 2>&1 | tail -n 1 > "$RESULTS_FOLDER/osrm_extract.bench"
    /usr/bin/time -f "Time: %E Peak RAM: %M Kb" $BINARIES_FOLDER/osrm-partition $FOLDER/data.osrm 2>&1 | tail -n 1 > "$RESULTS_FOLDER/osrm_partition.bench"
    /usr/bin/time -f "Time: %E Peak RAM: %M Kb" $BINARIES_FOLDER/osrm-customize $FOLDER/data.osrm 2>&1 | tail -n 1 > "$RESULTS_FOLDER/osrm_customize.bench"
    /usr/bin/time -f "Time: %E Peak RAM: %M Kb" $BINARIES_FOLDER/osrm-contract $FOLDER/data.osrm 2>&1 | tail -n 1 > "$RESULTS_FOLDER/osrm_contract.bench"

    if [ -f "$FOLDER/scripts/ci/locustfile.py" ]; then
        for ALGORITHM in mld ch; do
            $BINARIES_FOLDER/osrm-routed --algorithm $ALGORITHM $FOLDER/data.osrm &
            OSRM_ROUTED_PID=$!

            # wait for osrm-routed to start
            curl --retry-delay 3 --retry 10 --retry-all-errors "http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true"
            locust -f $FOLDER/scripts/ci/locustfile.py \
                --headless \
                --processes -1 \
                --users 10 \
                --spawn-rate 1 \
                --host http://localhost:5000 \
                --run-time 1m \
                --csv=locust_results_$ALGORITHM \
                --loglevel ERROR

            python3 $FOLDER/scripts/ci/process_locust_benchmark_results.py locust_results_$ALGORITHM $ALGORITHM $RESULTS_FOLDER


            kill -0 $OSRM_ROUTED_PID
        done
    fi

}

run_benchmarks_for_folder $1 "${1}_results"
run_benchmarks_for_folder $2 "${2}_results"

