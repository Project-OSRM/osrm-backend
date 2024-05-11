#!/bin/bash
set -eou pipefail

function run_benchmarks_for_folder {
    echo "Running benchmarks for $1"

    FOLDER=$1
    RESULTS_FOLDER=$2

    mkdir -p $RESULTS_FOLDER

    BENCHMARKS_FOLDER="$FOLDER/build/src/benchmarks"

    ./$BENCHMARKS_FOLDER/match-bench "./$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/match_mld.bench"
    ./$BENCHMARKS_FOLDER/match-bench "./$FOLDER/test/data/ch/monaco.osrm" > "$RESULTS_FOLDER/match_ch.bench"
    ./$BENCHMARKS_FOLDER/alias-bench > "$RESULTS_FOLDER/alias.bench"
    ./$BENCHMARKS_FOLDER/json-render-bench  "./$FOLDER/src/benchmarks/portugal_to_korea.json" > "$RESULTS_FOLDER/json-render.bench"
    ./$BENCHMARKS_FOLDER/packedvector-bench > "$RESULTS_FOLDER/packedvector.bench"
    ./$BENCHMARKS_FOLDER/rtree-bench "./$FOLDER/test/data/monaco.osrm.ramIndex" "./$FOLDER/test/data/monaco.osrm.fileIndex" "./$FOLDER/test/data/monaco.osrm.nbg_nodes" > "$RESULTS_FOLDER/rtree.bench"
}

run_benchmarks_for_folder $1 "${1}_results"
run_benchmarks_for_folder $2 "${2}_results"

