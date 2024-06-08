#!/bin/bash
set -eou pipefail

function measure_peak_ram_and_time {
    COMMAND=$1
    OUTPUT_FILE=$2

    OUTPUT=$(/usr/bin/time -f "%e %M" $COMMAND 2>&1 | tail -n 1)

    TIME=$(echo $OUTPUT | awk '{print $1}')
    PEAK_RAM_KB=$(echo $OUTPUT | awk '{print $2}')
    PEAK_RAM_MB=$(echo "scale=2; $PEAK_RAM_KB / 1024" | bc)
    echo "Time: ${TIME}s Peak RAM: ${PEAK_RAM_MB}MB" > $OUTPUT_FILE
}

function run_benchmarks_for_folder {
    echo "Running benchmarks for $1"

    FOLDER=$1
    RESULTS_FOLDER=$2
    SCRIPTS_FOLDER=$3

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

    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-extract -p $FOLDER/profiles/car.lua $FOLDER/data.osm.pbf" "$RESULTS_FOLDER/osrm_extract.bench"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-partition $FOLDER/data.osrm" "$RESULTS_FOLDER/osrm_partition.bench"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-customize $FOLDER/data.osrm" "$RESULTS_FOLDER/osrm_customize.bench"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-contract $FOLDER/data.osrm" "$RESULTS_FOLDER/osrm_contract.bench"

    for ALGORITHM in ch mld; do
        $BINARIES_FOLDER/osrm-routed --algorithm $ALGORITHM $FOLDER/data.osrm &
        OSRM_ROUTED_PID=$!

        # wait for osrm-routed to start
        curl --retry-delay 3 --retry 10 --retry-all-errors "http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true"

        for METHOD in route nearest trip table match; do
            python3 $SCRIPTS_FOLDER/scripts/ci/e2e_benchmark.py --host http://localhost:5000 --method $METHOD --num_requests 1000 > $RESULTS_FOLDER/e2e_${METHOD}_${ALGORITHM}.bench
        done

        kill -0 $OSRM_ROUTED_PID
    done


}

run_benchmarks_for_folder $1 "${1}_results" $2
run_benchmarks_for_folder $2 "${2}_results" $2

