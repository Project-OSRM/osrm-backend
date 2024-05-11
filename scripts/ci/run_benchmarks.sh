#!/bin/sh
set -e pipefail

function run_benchmarks_for_folder {
    echo "Running benchmarks for $1"

    FOLDER=$1
    RESULTS_FOLDER=$2

    mkdir -p $RESULTS_FOLDER

    ./$FOLDER/build/src/benchmarks/match-bench "./$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/match_mld.bench"
    ./$FOLDER/build/src/benchmarks/match-bench "./$FOLDER/test/data/ch/monaco.osrm" > "$RESULTS_FOLDER/match_ch.bench"
    ./$FOLDER/build/src/benchmarks/alias-bench > "$RESULTS_FOLDER/alias.bench"
    ./$FOLDER/build/src/benchmarks/json-render-bench  "./$FOLDER/src/benchmarks/portugal_to_korea.json" > "$RESULTS_FOLDER/json-render.bench"
    ./$FOLDER/build/src/benchmarks/packedvector-bench > "$RESULTS_FOLDER/packedvector.bench"
    ./$FOLDER/build/src/benchmarks/rtree-bench "./$FOLDER/test/data/monaco.osrm.ramIndex" "./$FOLDER/test/data/monaco.osrm.fileIndex" "./$FOLDER/test/data/monaco.osrm.nbg_nodes" > "$RESULTS_FOLDER/rtree.bench"
}

run_benchmarks_for_folder $1 "${1}_results"
run_benchmarks_for_folder $2 "${2}_results"

