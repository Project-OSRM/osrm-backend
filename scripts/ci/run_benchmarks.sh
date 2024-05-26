#!/bin/bash
set -eou pipefail

function run_benchmarks_for_folder {
    echo "Running benchmarks for $1"

    FOLDER=$1
    RESULTS_FOLDER=$2

    mkdir -p $RESULTS_FOLDER

    BENCHMARKS_FOLDER="$FOLDER/build/src/benchmarks"

    # ./$BENCHMARKS_FOLDER/match-bench "./$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/match_mld.bench"
    # ./$BENCHMARKS_FOLDER/match-bench "./$FOLDER/test/data/ch/monaco.osrm" ch > "$RESULTS_FOLDER/match_ch.bench"
    # ./$BENCHMARKS_FOLDER/route-bench "./$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/route_mld.bench" || true # TODO: remove `true` when this benchmark will be merged to master
    # ./$BENCHMARKS_FOLDER/route-bench "./$FOLDER/test/data/ch/monaco.osrm" ch > "$RESULTS_FOLDER/route_ch.bench" || true # TODO: remove `true` when this benchmark will be merged to master
    # ./$BENCHMARKS_FOLDER/alias-bench > "$RESULTS_FOLDER/alias.bench"
    # ./$BENCHMARKS_FOLDER/json-render-bench  "./$FOLDER/src/benchmarks/portugal_to_korea.json" > "$RESULTS_FOLDER/json-render.bench"
    # ./$BENCHMARKS_FOLDER/packedvector-bench > "$RESULTS_FOLDER/packedvector.bench"
    # ./$BENCHMARKS_FOLDER/rtree-bench "./$FOLDER/test/data/monaco.osrm.ramIndex" "./$FOLDER/test/data/monaco.osrm.fileIndex" "./$FOLDER/test/data/monaco.osrm.nbg_nodes" > "$RESULTS_FOLDER/rtree.bench"

    BINARIES_FOLDER="$FOLDER/build"
    echo "PWD: $FOLDER"
    cp ~/data.osm.pbf $FOLDER
    $BINARIES_FOLDER/osrm-extract -p $FOLDER/profiles/car.lua $FOLDER/data.osm.pbf
    $BINARIES_FOLDER/osrm-partition $FOLDER/data.osrm
    $BINARIES_FOLDER/osrm-customize $FOLDER/data.osrm
    $BINARIES_FOLDER/osrm-routed --algorithm mld $FOLDER/data.osrm &
    OSRM_ROUTED_PID=$!

    # TODO: save results
    curl --retry-delay 3 --retry 10 --retry-all-errors "http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true"
    locust -f $FOLDER/locustfile.py --headless --users 10 --spawn-rate 1 --host http://localhost:5000 --run-time 1m --csv=results

    echo "STATS: "
    cat results_stats.csv
    echo "FAILURES: "
    cat results_failures.csv
    echo "EXCEPTIONS: "
    cat results_exceptions.csv
    echo "STATS HISTORY: "
    cat results_stats_history.csv


    kill -0 $OSRM_ROUTED_PID
}

run_benchmarks_for_folder $1 "${1}_results"
run_benchmarks_for_folder $2 "${2}_results"

