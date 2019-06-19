#!/bin/bash -x
BUILD_PATH=${BUILD_PATH:="/osrm-build"}
DATA_PATH=${DATA_PATH:="/osrm-data"}
OSRM_EXTRA_COMMAND="-l --verbosity"
MAPDATA_NAME_WITH_SUFFIX=map

_sig() {
  kill -TERM $child 2>/dev/null
}

if [ "$1" = 'routed_startup' ]; then
  trap _sig SIGKILL SIGTERM SIGHUP SIGINT EXIT

  TRAFFIC_FILE=traffic.csv
  TRAFFIC_PROXY_IP=${2:-"10.189.102.81"}

  cd ${DATA_PATH}
  ${BUILD_PATH}/osrm_traffic_updater -c ${TRAFFIC_PROXY_IP} -d=false -f ${TRAFFIC_FILE}
  ls -lh
  ${BUILD_PATH}/osrm-customize ${MAPDATA_NAME_WITH_SUFFIX}.osrm  --segment-speed-file ${TRAFFIC_FILE} ${OSRM_EXTRA_COMMAND}
  ${BUILD_PATH}/osrm-routed ${MAPDATA_NAME_WITH_SUFFIX}.osrm -a MLD --max-table-size 8000 &
  child=$!
  wait "$child"

elif [ "$1" = 'compile_mapdata' ]; then
  trap _sig SIGKILL SIGTERM SIGHUP SIGINT EXIT

  PBF_FILE_URL=${2}
  KEEP_COMPILED_DATA=${3:-"false"}
  GENERATE_DATA_PACKAGE=${4:-"false"}

  curl ${PBF_FILE_URL} > $DATA_PATH/${MAPDATA_NAME_WITH_SUFFIX}.osm.pbf
  ${BUILD_PATH}/osrm-extract $DATA_PATH/${MAPDATA_NAME_WITH_SUFFIX}.osm.pbf -p ${BUILD_PATH}/profiles/car.lua ${OSRM_EXTRA_COMMAND}
  ${BUILD_PATH}/osrm-partition $DATA_PATH/${MAPDATA_NAME_WITH_SUFFIX}.osrm ${OSRM_EXTRA_COMMAND}
  ${BUILD_PATH}/osrm-customize $DATA_PATH/${MAPDATA_NAME_WITH_SUFFIX}.osrm ${OSRM_EXTRA_COMMAND}
  
  # clean source pbf and temp .osrm
  rm -f $DATA_PATH/${MAPDATA_NAME_WITH_SUFFIX}.osm.pbf
  rm -f $DATA_PATH/${MAPDATA_NAME_WITH_SUFFIX}.osrm

  # package and publish compiled mapdata 
  if [ ${GENERATE_DATA_PACKAGE} == "true" ]; then
    cd ${DATA_PATH}
    tar -zcf ${MAPDATA_NAME_WITH_SUFFIX}.tar.gz *
    
    SAVE_DATA_PACKAGE_PATH=/save-data
    mkdir -p ${SAVE_DATA_PACKAGE_PATH}
    mv ${DATA_PATH}/${MAPDATA_NAME_WITH_SUFFIX}.tar.gz ${SAVE_DATA_PACKAGE_PATH}/
  fi

  # rm compiled data if not needed
  if [ ${KEEP_COMPILED_DATA} != "true" ]; then
    rm -f $DATA_PATH/*
  fi

else
  exec "$@"
fi
