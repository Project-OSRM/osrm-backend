#!/bin/bash
set -eou pipefail

function usage {
    echo "Usage: $0 -f <folder> -r <results_folder> -s <scripts_folder> -b <binaries_folder> -o <osm_pbf> -g <gps_traces>"
    exit 1
}

while getopts ":f:r:s:b:o:g:" opt; do
  case $opt in
    f) FOLDER="$OPTARG"
    ;;
    r) RESULTS_FOLDER="$OPTARG"
    ;;
    s) SCRIPTS_FOLDER="$OPTARG"
    ;;
    b) BINARIES_FOLDER="$OPTARG"
    ;;
    o) OSM_PBF="$OPTARG"
    ;;
    g) GPS_TRACES="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
        usage
    ;;
    :) echo "Option -$OPTARG requires an argument." >&2
        usage
    ;;
  esac
done

if [ -z "${FOLDER:-}" ] || [ -z "${RESULTS_FOLDER:-}" ] || [ -z "${SCRIPTS_FOLDER:-}" ] || [ -z "${BINARIES_FOLDER:-}" ] || [ -z "${OSM_PBF:-}" ] || [ -z "${GPS_TRACES:-}" ]; then
    usage
fi

function measure_peak_ram_and_time {
    COMMAND=$1
    OUTPUT_FILE=$2
    if [ "$(uname)" == "Darwin" ]; then
        # on macOS time has different parameters, so simply run command on macOS
        $COMMAND > /dev/null 2>&1
    else
        OUTPUT=$(/usr/bin/time -f "%e %M" $COMMAND 2>&1 | tail -n 1)

        TIME=$(echo $OUTPUT | awk '{print $1}')
        PEAK_RAM_KB=$(echo $OUTPUT | awk '{print $2}')
        PEAK_RAM_MB=$(echo "scale=2; $PEAK_RAM_KB / 1024" | bc)
        echo "Time: ${TIME}s Peak RAM: ${PEAK_RAM_MB}MB" > $OUTPUT_FILE
    fi
}

function run_benchmarks_for_folder {
    mkdir -p $RESULTS_FOLDER

    BENCHMARKS_FOLDER="$BINARIES_FOLDER/src/benchmarks"

    echo "Running match-bench MLD"
    $BENCHMARKS_FOLDER/match-bench "$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/match_mld.bench"
    echo "Running match-bench CH"
    $BENCHMARKS_FOLDER/match-bench "$FOLDER/test/data/ch/monaco.osrm" ch > "$RESULTS_FOLDER/match_ch.bench"
    echo "Running route-bench MLD"
    $BENCHMARKS_FOLDER/route-bench "$FOLDER/test/data/mld/monaco.osrm" mld > "$RESULTS_FOLDER/route_mld.bench"
    echo "Running route-bench CH"
    $BENCHMARKS_FOLDER/route-bench "$FOLDER/test/data/ch/monaco.osrm" ch > "$RESULTS_FOLDER/route_ch.bench"
    echo "Running alias"
    $BENCHMARKS_FOLDER/alias-bench > "$RESULTS_FOLDER/alias.bench"
    echo "Running json-render-bench"
    $BENCHMARKS_FOLDER/json-render-bench  "$FOLDER/test/data/portugal_to_korea.json" > "$RESULTS_FOLDER/json-render.bench"
    echo "Running packedvector-bench"
    $BENCHMARKS_FOLDER/packedvector-bench > "$RESULTS_FOLDER/packedvector.bench"
    echo "Running rtree-bench"
    $BENCHMARKS_FOLDER/rtree-bench "$FOLDER/test/data/monaco.osrm.ramIndex" "$FOLDER/test/data/monaco.osrm.fileIndex" "$FOLDER/test/data/monaco.osrm.nbg_nodes" > "$RESULTS_FOLDER/rtree.bench"

    cp -rf $OSM_PBF $FOLDER/data.osm.pbf

    echo "Running osrm-extract"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-extract -p $FOLDER/profiles/car.lua $FOLDER/data.osm.pbf" "$RESULTS_FOLDER/osrm_extract.bench"
    echo "Running osrm-partition"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-partition $FOLDER/data.osrm" "$RESULTS_FOLDER/osrm_partition.bench"
    echo "Running osrm-customize"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-customize $FOLDER/data.osrm" "$RESULTS_FOLDER/osrm_customize.bench"
    echo "Running osrm-contract"
    measure_peak_ram_and_time "$BINARIES_FOLDER/osrm-contract $FOLDER/data.osrm" "$RESULTS_FOLDER/osrm_contract.bench"


    for ALGORITHM in ch mld; do
        for BENCH in nearest table trip route match; do
            echo "Running node $BENCH $ALGORITHM"
            START=$(date +%s.%N)
            node $SCRIPTS_FOLDER/scripts/ci/bench.js $FOLDER/lib/binding/node_osrm.node $FOLDER/data.osrm $ALGORITHM $BENCH $GPS_TRACES > "$RESULTS_FOLDER/node_${BENCH}_${ALGORITHM}.bench" 5
            END=$(date +%s.%N)
            DIFF=$(echo "$END - $START" | bc)
            echo "Took: ${DIFF}s"
        done
    done
    
    for ALGORITHM in ch mld; do
        for BENCH in nearest table trip route match; do
            echo "Running random $BENCH $ALGORITHM"
            START=$(date +%s.%N)
            $BENCHMARKS_FOLDER/bench "$FOLDER/data.osrm" $ALGORITHM $GPS_TRACES ${BENCH} > "$RESULTS_FOLDER/random_${BENCH}_${ALGORITHM}.bench" 5 || true
            END=$(date +%s.%N)
            DIFF=$(echo "$END - $START" | bc)
            echo "Took: ${DIFF}s"
        done
    done


    for ALGORITHM in ch mld; do
        $BINARIES_FOLDER/osrm-routed --algorithm $ALGORITHM $FOLDER/data.osrm > /dev/null 2>&1 &
        OSRM_ROUTED_PID=$!

        # wait for osrm-routed to start
        if ! curl --retry-delay 3 --retry 10 --retry-all-errors "http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true" > /dev/null 2>&1; then
            echo "osrm-routed failed to start for algorithm $ALGORITHM"
            kill -9 $OSRM_ROUTED_PID
            continue
        fi

        for METHOD in route nearest trip table match; do
            echo "Running e2e benchmark for $METHOD $ALGORITHM"
            START=$(date +%s.%N)
            python3 $SCRIPTS_FOLDER/scripts/ci/e2e_benchmark.py --host http://localhost:5000 --method $METHOD --iterations 5 --num_requests 1000 --gps_traces_file_path $GPS_TRACES > $RESULTS_FOLDER/e2e_${METHOD}_${ALGORITHM}.bench
            END=$(date +%s.%N)
            DIFF=$(echo "$END - $START" | bc)
            echo "Took: ${DIFF}s"
        done

        kill -9 $OSRM_ROUTED_PID
    done
}

run_benchmarks_for_folder

