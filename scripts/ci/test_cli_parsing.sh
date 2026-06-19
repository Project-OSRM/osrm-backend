#!/bin/bash
set -euo pipefail

# Integration tests for CLI argument parsing across all six OSRM tool binaries.
# Strategy: negative observation. We rarely run anything that succeeds —
# instead we send each tool a known-bad input AFTER parsing and check that the
# tool's response proves a specific parse path was taken. A tool that gets to
# "Required files are missing" or "Input file ... not found" has parsed its
# args; a tool that errors before that has rejected its args.

function usage {
    echo "Usage: $0 -b <binaries_folder>"
    exit 1
}

while getopts ":b:" opt; do
    case $opt in
        b) BINARIES_FOLDER="$OPTARG" ;;
        \?) echo "Invalid option -$OPTARG" >&2; usage ;;
        :) echo "Option -$OPTARG requires an argument." >&2; usage ;;
    esac
done

if [ -z "${BINARIES_FOLDER:-}" ]; then
    usage
fi

if [ ! -d "$BINARIES_FOLDER" ]; then
    echo "Binaries folder not found: $BINARIES_FOLDER" >&2
    exit 1
fi

EXTRACT="$BINARIES_FOLDER/osrm-extract"
CONTRACT="$BINARIES_FOLDER/osrm-contract"
CUSTOMIZE="$BINARIES_FOLDER/osrm-customize"
PARTITION="$BINARIES_FOLDER/osrm-partition"
ROUTED="$BINARIES_FOLDER/osrm-routed"
DATASTORE="$BINARIES_FOLDER/osrm-datastore"

for bin in "$EXTRACT" "$CONTRACT" "$CUSTOMIZE" "$PARTITION" "$ROUTED" "$DATASTORE"; do
    if [ ! -x "$bin" ]; then
        echo "Binary missing or not executable: $bin" >&2
        exit 1
    fi
done

PASS=0
FAIL=0
FAILED_NAMES=()

pass() {
    PASS=$((PASS + 1))
    echo "PASS: $1"
}

fail() {
    FAIL=$((FAIL + 1))
    FAILED_NAMES+=("$1")
    echo "FAIL: $1" >&2
    if [ -n "${2:-}" ]; then
        echo "      $2" >&2
    fi
}

# Capture combined stdout+stderr; never fail the script on the SUT's exit code.
run_capture() {
    "$@" 2>&1 || true
}

# Run command, succeed if exit code is 0.
assert_exit_zero() {
    local name=$1; shift
    if "$@" >/dev/null 2>&1; then
        pass "$name"
    else
        fail "$name" "expected exit 0, got $?"
    fi
}

# Run command, succeed if exit code is non-zero.
assert_exit_nonzero() {
    local name=$1; shift
    if "$@" >/dev/null 2>&1; then
        fail "$name" "expected non-zero exit"
    else
        pass "$name"
    fi
}

# Run command, succeed if combined output matches the regex.
assert_output_matches() {
    local name=$1 regex=$2; shift 2
    local output
    output=$(run_capture "$@")
    if echo "$output" | grep -Eq "$regex"; then
        pass "$name"
    else
        fail "$name" "output did not match /$regex/"
        echo "      got: $(echo "$output" | head -c 400)" >&2
    fi
}

# Run command, succeed if combined output does NOT match the regex.
assert_output_not_matches() {
    local name=$1 regex=$2; shift 2
    local output
    output=$(run_capture "$@")
    if echo "$output" | grep -Eq "$regex"; then
        fail "$name" "output unexpectedly matched /$regex/"
        echo "      got: $(echo "$output" | head -c 400)" >&2
    else
        pass "$name"
    fi
}

NONEXISTENT=/nonexistent.osrm

# --- help / version, all six tools -----------------------------------------

assert_exit_zero "extract/--help"      "$EXTRACT"   --help
assert_exit_zero "extract/--version"   "$EXTRACT"   --version
assert_exit_zero "contract/--help"     "$CONTRACT"  --help
assert_exit_zero "contract/--version"  "$CONTRACT"  --version
assert_exit_zero "customize/--help"    "$CUSTOMIZE" --help
assert_exit_zero "customize/--version" "$CUSTOMIZE" --version
assert_exit_zero "partition/--help"    "$PARTITION" --help
assert_exit_zero "partition/--version" "$PARTITION" --version
assert_exit_zero "routed/--help"       "$ROUTED"    --help
assert_exit_zero "routed/--version"    "$ROUTED"    --version
assert_exit_zero "datastore/--help"    "$DATASTORE" --help
assert_exit_zero "datastore/--version" "$DATASTORE" --version

# --list-inputs is a parse-only path; extract has no required input files
# beyond the OSM input itself, so output is empty — just check exit 0.
# (contract/customize have updater_config files; tested implicitly via the
# end-to-end pipeline in `make -C test/data benchmark`.)
assert_exit_zero "extract/--list-inputs" "$EXTRACT" --list-inputs

# --- algorithm transform on routed -----------------------------------------

# Default (no --algorithm) must parse and reach engine init. If default_val("CH")
# does not flow through the transform, the engine init would fail differently.
assert_output_matches "routed/default-algorithm-parses-to-engine-init" \
    "Required files are missing" "$ROUTED" "$NONEXISTENT"

# Case-insensitive lowercase alias must parse.
assert_output_matches "routed/--algorithm mld parses (case-insensitive)" \
    "Required files are missing" "$ROUTED" --algorithm mld "$NONEXISTENT"

# Mixed case must parse.
assert_output_matches "routed/--algorithm Ch parses (mixed-case)" \
    "Required files are missing" "$ROUTED" --algorithm Ch "$NONEXISTENT"

# Garbage must NOT reach engine init (parse rejected).
assert_exit_nonzero "routed/--algorithm bogus exits nonzero" \
    "$ROUTED" --algorithm bogus "$NONEXISTENT"
assert_output_not_matches "routed/--algorithm bogus rejected before engine init" \
    "Required files are missing" "$ROUTED" --algorithm bogus "$NONEXISTENT"

# --- "unlimited" double transform on routed --------------------------------

assert_output_matches "routed/--max-matching-radius unlimited parses" \
    "Required files are missing" \
    "$ROUTED" --max-matching-radius unlimited "$NONEXISTENT"

assert_exit_nonzero "routed/--max-matching-radius zzzz exits nonzero" \
    "$ROUTED" --max-matching-radius zzzz "$NONEXISTENT"
assert_output_not_matches "routed/--max-matching-radius zzzz rejected" \
    "Required files are missing" \
    "$ROUTED" --max-matching-radius zzzz "$NONEXISTENT"

# --- feature-dataset multitoken on routed ----------------------------------

# Multitoken: multiple values per single flag invocation must parse. Note: the
# positional must come BEFORE --disable-feature-dataset, otherwise boost::po's
# multitoken greedily consumes the positional as a (bogus) dataset value.
assert_output_matches "routed/--disable-feature-dataset multitoken parses" \
    "Required files are missing" \
    "$ROUTED" "$NONEXISTENT" --disable-feature-dataset ROUTE_STEPS ROUTE_GEOMETRY

# Single value variant (same ordering rule).
assert_output_matches "routed/--disable-feature-dataset single value parses" \
    "Required files are missing" \
    "$ROUTED" "$NONEXISTENT" --disable-feature-dataset ROUTE_STEPS

assert_exit_nonzero "routed/--disable-feature-dataset bogus exits nonzero" \
    "$ROUTED" "$NONEXISTENT" --disable-feature-dataset bogus
assert_output_not_matches "routed/--disable-feature-dataset bogus rejected" \
    "Required files are missing" \
    "$ROUTED" "$NONEXISTENT" --disable-feature-dataset bogus

# --- max-cell-sizes manual comma-split + sort check on partition ----------

# Comma-separated split into vector must parse and pass the sort check.
assert_output_matches "partition/--max-cell-sizes 128,256,512 parses" \
    "not found" \
    "$PARTITION" --max-cell-sizes 128,256,512 /nonexistent

# Sort check must reject unsorted input with the specific error message.
assert_output_matches "partition/--max-cell-sizes 512,256 fails sort check" \
    "must be sorted in non-descending order" \
    "$PARTITION" --max-cell-sizes 512,256 /nonexistent

# Non-numeric token must be rejected at parse time.
assert_exit_nonzero "partition/--max-cell-sizes 1,abc exits nonzero" \
    "$PARTITION" --max-cell-sizes 1,abc /nonexistent

# --- path with spaces (the original bug) -----------------------------------

# Use a path that does not exist; each tool's error log echoes back the path
# string. If parsing truncated at whitespace (the original boost::po bug),
# the echo would not contain the part of the path after the space. The shim
# is centralized, so testing a positional fs::path on more than one tool
# guards against future per-tool regressions (e.g. a missing #include).
SPACE_DIR="/tmp/osrm cli test space"
assert_output_matches "extract/path-with-spaces preserved end-to-end" \
    "$SPACE_DIR/missing\\.osm\\.pbf" \
    "$EXTRACT" --profile /nonexistent.lua "$SPACE_DIR/missing.osm.pbf"

assert_output_matches "partition/path-with-spaces preserved end-to-end" \
    "$SPACE_DIR/missing\\.osrm" \
    "$PARTITION" "$SPACE_DIR/missing.osrm"

# --- unknown option rejection ----------------------------------------------

assert_exit_nonzero "extract/--bogus-flag exits nonzero" \
    "$EXTRACT" --bogus-flag /nonexistent

assert_exit_nonzero "routed/--bogus-flag exits nonzero" \
    "$ROUTED" --bogus-flag "$NONEXISTENT"

# --- summary ---------------------------------------------------------------

echo
echo "----------------------------------------"
echo "$PASS passed, $FAIL failed"
if [ "$FAIL" -gt 0 ]; then
    echo "Failed tests:"
    for n in "${FAILED_NAMES[@]}"; do
        echo "  - $n"
    done
    exit 1
fi
