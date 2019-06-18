#!/bin/bash
DATA_PATH=${DATA_PATH:="/osrm-data"}

_sig() {
  kill -TERM $child 2>/dev/null
}

if [ "$1" = 'routed_startup' ]; then
  trap _sig SIGKILL SIGTERM SIGHUP SIGINT EXIT
  ./osrm-routed $DATA_PATH/$2.osrm -a MLD --max-table-size 8000 &
  child=$!
  wait "$child"
elif [ "$1" = 'compile_mapdata' ]; then
  trap _sig SIGKILL SIGTERM SIGHUP SIGINT EXIT
  if [ ! -f $DATA_PATH/$2.osrm ]; then
    if [ ! -f $DATA_PATH/$2.osm.pbf ]; then
      curl $3 > $DATA_PATH/$2.osm.pbf
    fi
    ./osrm-extract $DATA_PATH/$2.osm.pbf -p profiles/car.lua
    ./osrm-partition $DATA_PATH/$2.osrm
    ./osrm-customize $DATA_PATH/$2.osrm
  fi  
else
  exec "$@"
fi
