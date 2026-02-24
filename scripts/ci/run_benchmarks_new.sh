#!/bin/bash
set -e -o pipefail

source build/osrm-run-env.sh

SCRIPTS_DIR="${PROJECT_SOURCE_DIR}/scripts/ci"
LOGS_DIR="${PROJECT_SOURCE_DIR}/test/logs"

MONACO="$OSRM_TEST_DATA_DIR"

DATASET=berlin
PBF="${OSRM_TEST_DATA_DIR}/${DATASET}.osm.pbf"
GPS_TRACES="${OSRM_TEST_DATA_DIR}/${DATASET}_gps_traces.csv.gz"
DATA_DIR="${OSRM_TEST_DATA_DIR}"
DATA="${DATASET}.osrm"

mkdir -p "$LOGS_DIR"
mkdir -p "$DATA_DIR"

function measure_peak_ram_and_time {
    echo "Running $1"

    MSG='Time: %es Peak RAM: %MKB'

    case $(uname) in
      Linux)
        TIME=(/usr/bin/time -f "$MSG" -o "$LOGS_DIR/$1.bench")
      ;;
      Darwin)
        TIME=(gtime -f "$MSG" -o "$LOGS_DIR/$1.bench")
      ;;
      *)
        TIME=()
      ;;
    esac

    "${TIME[@]}" $OSRM_BUILD_DIR/$@ > /dev/null 2>&1
    cat "$LOGS_DIR/$1.bench" || true
}

function summary {
    if [ -n "$GITHUB_STEP_SUMMARY" ]; then
        printf '%b' "$1" >> $GITHUB_STEP_SUMMARY
    fi
}

for method in route match; do
    for algorithm in ch mld; do
        echo "Running ${method}-bench ${algorithm}"
        "$OSRM_BENCHMARKS_BUILD_DIR/${method}-bench" "$MONACO/${algorithm}/monaco.osrm" ${algorithm} \
            > "$LOGS_DIR/${method}_${algorithm}.bench"
    done
done

echo "Running alias"
"$OSRM_BENCHMARKS_BUILD_DIR/alias-bench" > "$LOGS_DIR/alias.bench"
echo "Running json-render-bench"
"$OSRM_BENCHMARKS_BUILD_DIR/json-render-bench"  "$OSRM_TEST_DATA_DIR/portugal_to_korea.json" > "$LOGS_DIR/json-render.bench"
echo "Running packedvector-bench"
"$OSRM_BENCHMARKS_BUILD_DIR/packedvector-bench" 10 100000 > "$LOGS_DIR/packedvector.bench"
echo "Running rtree-bench"
"$OSRM_BENCHMARKS_BUILD_DIR/rtree-bench" \
    "$MONACO/ch/monaco.osrm.ramIndex" \
    "$MONACO/ch/monaco.osrm.fileIndex" \
    "$MONACO/ch/monaco.osrm.nbg_nodes" > "$LOGS_DIR/rtree.bench"

pushd "$DATA_DIR"

measure_peak_ram_and_time osrm-extract -p "$PROJECT_SOURCE_DIR/profiles/car.lua" "$PBF"
measure_peak_ram_and_time osrm-partition "$DATA"
measure_peak_ram_and_time osrm-customize "$DATA"
measure_peak_ram_and_time osrm-contract  "$DATA"

if [ -z "$GITHUB_STEP_SUMMARY" ]; then
    # these waste an awful lot of time on CI
    # FIXME: reenable after parallelizing them
    if [[ -f "$OSRM_NODEJS_INSTALL_DIR/lib/index.js" ]]; then
        for ALGORITHM in ch mld; do
            for METHOD in nearest table trip route match; do
                echo "Running node $METHOD $ALGORITHM"
                node "$SCRIPTS_DIR/bench.js" "$OSRM_NODEJS_INSTALL_DIR/lib/index.js" "$DATA" $ALGORITHM $METHOD 5 "$GPS_TRACES" \
                    > "$LOGS_DIR/node_${METHOD}_${ALGORITHM}.bench" || true
            done
        done
    fi

    for ALGORITHM in ch mld; do
        for METHOD in nearest table trip route match; do
            echo "Running random $METHOD $ALGORITHM"
            "$OSRM_BENCHMARKS_BUILD_DIR/bench" "$DATA" $ALGORITHM $METHOD 5 "$GPS_TRACES" \
                > "$LOGS_DIR/random_${METHOD}_${ALGORITHM}.bench" || true
        done
    done
fi

popd
