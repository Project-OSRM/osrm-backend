#!/usr/bin/env bash
#
# Highway Mode 2 — OSRM data pipeline.
#
# Downloads a Geofabrik OSM extract, verifies it, runs the OSRM MLD pipeline
# (extract → partition → customize) into a dated, versioned output directory, and
# atomically swaps a `current` symlink to point at it. Old datasets are pruned to a
# retention count. A failure at any stage leaves the existing `current` untouched, so a
# running osrm-routed keeps serving the last-good dataset.
#
# Intended to run monthly (k8s CronJob, deploy/k8s/osrm-pipeline.cronjob.yaml) or by hand.
# The serving Deployment must reload/restart to pick up a new `current` (osrm-routed does
# not hot-reload a symlink swap; the CronJob triggers a rollout — see deploy/k8s README).
#
# Env:
#   REGION           Geofabrik path without extension   (default: europe/netherlands)
#   OSRM_DATA_DIR    root data dir                       (default: /data)
#   OSRM_PROFILE     Lua profile                         (default: /opt/profiles/car.lua)
#   RETENTION        versioned datasets to keep          (default: 2)
#   MIN_FREE_GB      abort if less free space than this  (default: 12)
#   GEOFABRIK_BASE   mirror base URL                     (default: https://download.geofabrik.de)
#
# Layout produced:
#   $OSRM_DATA_DIR/datasets/<UTC-timestamp>-<region>/data.osrm*
#   $OSRM_DATA_DIR/current -> datasets/<UTC-timestamp>-<region>   (atomic symlink)
#
set -euo pipefail

REGION="${REGION:-europe/netherlands}"
OSRM_DATA_DIR="${OSRM_DATA_DIR:-/data}"
OSRM_PROFILE="${OSRM_PROFILE:-/opt/profiles/car.lua}"
RETENTION="${RETENTION:-2}"
MIN_FREE_GB="${MIN_FREE_GB:-12}"
GEOFABRIK_BASE="${GEOFABRIK_BASE:-https://download.geofabrik.de}"

log()  { printf '%s [pipeline] %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" >&2; }
die()  { log "ERROR: $*"; exit 1; }

# --- preflight ------------------------------------------------------------------------
command -v osrm-extract   >/dev/null || die "osrm-extract not on PATH"
command -v osrm-partition >/dev/null || die "osrm-partition not on PATH"
command -v osrm-customize >/dev/null || die "osrm-customize not on PATH"
command -v curl           >/dev/null || die "curl not on PATH"
[ -f "$OSRM_PROFILE" ]               || die "profile not found: $OSRM_PROFILE"

REGION_SLUG="$(basename "$REGION")"                       # europe/netherlands -> netherlands
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DATASETS_DIR="$OSRM_DATA_DIR/datasets"
WORK_DIR="$DATASETS_DIR/.work-$STAMP-$REGION_SLUG"        # dotted = in-progress, ignored by prune
FINAL_DIR="$DATASETS_DIR/$STAMP-$REGION_SLUG"
CURRENT_LINK="$OSRM_DATA_DIR/current"
PBF_URL="$GEOFABRIK_BASE/$REGION-latest.osm.pbf"
MD5_URL="$PBF_URL.md5"

mkdir -p "$DATASETS_DIR"

# Free-space check (need room for the .pbf + the .osrm artifacts, roughly 3–4x the pbf).
avail_kb="$(df -Pk "$DATASETS_DIR" | awk 'NR==2 {print $4}')"
avail_gb=$(( avail_kb / 1024 / 1024 ))
[ "$avail_gb" -ge "$MIN_FREE_GB" ] || die "only ${avail_gb}GB free in $DATASETS_DIR, need >= ${MIN_FREE_GB}GB"
log "region=$REGION  free=${avail_gb}GB  work=$WORK_DIR"

# Clean up a half-built work dir on any exit; the final dir + current link are only
# touched on the success path, so an early exit here never disturbs the live dataset.
cleanup() { [ -n "${WORK_DIR:-}" ] && rm -rf "$WORK_DIR"; }
trap cleanup EXIT

mkdir -p "$WORK_DIR"
PBF="$WORK_DIR/data.osm.pbf"

# --- download + verify ----------------------------------------------------------------
log "downloading $PBF_URL"
curl -fSL --retry 3 --retry-delay 5 -o "$PBF" "$PBF_URL" || die "download failed: $PBF_URL"

log "verifying md5 against $MD5_URL"
expected_md5="$(curl -fsSL --retry 3 "$MD5_URL" | awk '{print $1}')" || die "md5 fetch failed"
[ -n "$expected_md5" ] || die "empty md5 from $MD5_URL"
if command -v md5sum >/dev/null; then
    actual_md5="$(md5sum "$PBF" | awk '{print $1}')"
else
    actual_md5="$(md5 -q "$PBF")"   # macOS fallback for manual runs
fi
[ "$actual_md5" = "$expected_md5" ] || die "md5 mismatch: got $actual_md5 want $expected_md5"
log "md5 ok ($actual_md5)"

# --- OSRM MLD pipeline ----------------------------------------------------------------
# Everything writes inside WORK_DIR. osrm-extract emits data.osrm* alongside the pbf.
log "osrm-extract ($OSRM_PROFILE)"
osrm-extract   -p "$OSRM_PROFILE" "$PBF"
OSRM="$WORK_DIR/data.osrm"
[ -f "$OSRM" ] || die "extract produced no $OSRM"

log "osrm-partition"
osrm-partition "$OSRM"
log "osrm-customize"
osrm-customize "$OSRM"

# Drop the source pbf; the .osrm* set is self-contained for serving.
rm -f "$PBF"

# --- publish: atomic promote + symlink swap -------------------------------------------
# Promote work -> final (same filesystem, so mv is atomic), then atomically repoint
# `current`. ln -sfn writes a temp link and renames it, so a reader never sees a missing
# symlink. Only now is the live pointer changed.
mv "$WORK_DIR" "$FINAL_DIR"
trap - EXIT   # work dir no longer exists; nothing to clean

tmp_link="$CURRENT_LINK.tmp.$STAMP"
ln -s "$FINAL_DIR" "$tmp_link"
# `mv -T` renames the temp symlink over the live one atomically (GNU coreutils, the Linux
# container path). Fall back to `ln -sfn` for macOS manual runs where mv lacks -T.
mv -T "$tmp_link" "$CURRENT_LINK" 2>/dev/null || { ln -sfn "$FINAL_DIR" "$CURRENT_LINK"; rm -f "$tmp_link"; }
log "current -> $(readlink "$CURRENT_LINK")"

# --- retention: keep newest $RETENTION, never delete what `current` points to ---------
keep_target="$(readlink -f "$CURRENT_LINK" 2>/dev/null || readlink "$CURRENT_LINK")"
mapfile -t all < <(find "$DATASETS_DIR" -maxdepth 1 -mindepth 1 -type d \
                     -name '*-'"$REGION_SLUG" ! -name '.work-*' | sort -r)
idx=0
for d in "${all[@]}"; do
    idx=$((idx + 1))
    if [ "$idx" -le "$RETENTION" ] || [ "$(readlink -f "$d")" = "$keep_target" ]; then
        continue   # within the newest N, or the live dataset — keep either way
    fi
    log "pruning old dataset $d"
    rm -rf "$d"
done

log "done: $FINAL_DIR"
