#!/bin/sh

if [ -f /data/osrm_map_generated ]; then
  echo "OSRM data already prepared, skipping."
else
  echo "OSRM data not found, preparing..."
  osrm-extract -p /opt/car.lua /data/iran-latest.osm.pbf &&
  osrm-partition /data/iran-latest.osrm && 
  osrm-customize /data/iran-latest.osrm && 
  touch /data/osrm_map_generated   
fi

