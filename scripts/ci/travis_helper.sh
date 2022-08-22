#!/usr/bin/env bash

# This script is sourced, so do not set -e or -o pipefail here. Doing so would
# bleed into Travis' wrapper script, which messes with their workflow, e.g.
# preventing after_failure scripts to be triggered.

function mapbox_time_start {
    local name=$1
    mapbox_timer_name=$name

    mapbox_fold start $name

    mapbox_timer_id=$(printf %08x $(( RANDOM * RANDOM )))
    eval "mapbox_start_time_$mapbox_timer_id=$(mapbox_nanoseconds)"
    echo -en "travis_time:start:$mapbox_timer_id\n"
}

function mapbox_time_finish {
    local name=${1:-$mapbox_timer_name}
    local timer_id=${2:-$mapbox_timer_id}
    local timer_start="mapbox_start_time_$timer_id"
    eval local start_time=\${$timer_start}
    local end_time=$(mapbox_nanoseconds)
    local duration=$(($end_time-$start_time))
    echo -en "travis_time:end:$timer_id:start=$start_time,finish=$end_time,duration=$duration\n"
}

function mapbox_time {
    local name=$1 ; shift
    mapbox_time_start $name
    local timer_id=$mapbox_timer_id
    echo "\$ $@"
    # note: we capture the return code here
    # so that we can ensure mapbox_time_finish is called
    # and an error is trickled up correctly
    local RESULT=0
    $@ || RESULT=$?
    mapbox_time_finish $name $timer_id
    if [[ ${RESULT} != 0 ]]; then
      echo "$name failed with ${RESULT}"
      # note: we use false here instead of exit this this script is sourced
      # and exit would abort the shell which we don't want
      false
    else
      mapbox_fold end $name
    fi
}

function mapbox_fold {
  local action=$1
  local name=$2
  local ANSI_CLEAR="\e[0m"
  echo -en "travis_fold:${action}:${name}\r${ANSI_CLEAR}"
}

function mapbox_nanoseconds {
  local cmd="date"
  local format="+%s%N"
  local os=$(uname -s)

  if hash gdate > /dev/null 2>&1; then
    cmd="gdate" # use gdate if available
  elif [[ "$os" = Darwin ]]; then
    format="+%s000000000" # fallback to second precision on darwin (does not support %N)
  fi

  $cmd -u $format
}

export JOBS
export -f mapbox_fold
export -f mapbox_nanoseconds
export -f mapbox_time
export -f mapbox_time_start
export -f mapbox_time_finish
