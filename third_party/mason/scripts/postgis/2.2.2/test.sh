#!/usr/bin/env bash

set -ue

if [[ ${PGDATA:-unset} != "unset" ]] || [[ ${PGHOST:-unset} != "unset" ]] || [[ ${PGTEMP_DIR:-unset} != "unset" ]]; then
    echo "ERROR: this script deletes \${PGDATA}, \${PGHOST}, and \${PGTEMP_DIR}."
    echo "So it will not run if you have these set in your environment"
    exit 1
fi

export GDAL_PREFIX=$(../../../mason prefix gdal 2.0.2)
# make sure we can init, start, create db, and stop
export PGDATA=./local-postgres
# PGHOST must start with / so therefore must be absolute path
export PGHOST=$(pwd)/local-unix-socket
export PGTEMP_DIR=$(pwd)/local-tmp
export PGPORT=1111

# cleanup
function cleanup() {
    if [[ -d ${PGDATA} ]]; then rm -r ${PGDATA}; fi
    if [[ -d ${PGTEMP_DIR} ]]; then rm -r ${PGTEMP_DIR}; fi
    if [[ -d ${PGHOST} ]]; then rm -r ${PGHOST}; fi
    rm -f postgres.log
    rm -f seattle_washington_water_coast*
    rm -f seattle_washington.water.coast*
}

function setup() {
    mkdir ${PGTEMP_DIR}
    mkdir ${PGHOST}
}

function finish {
  ./mason_packages/.link/bin/pg_ctl -w stop
  cleanup
}

function pause(){
   read -p "$*"
}

trap finish EXIT

cleanup
setup

if [[ ! -d ./mason_packages/.link ]]; then
    ./script.sh link
fi

./mason_packages/.link/bin/initdb
export PATH=./mason_packages/.link/bin/:${PATH}
export GDAL_DATA=${GDAL_PREFIX}/share/gdal
postgres -k $PGHOST > postgres.log &
sleep 2
cat postgres.log
createdb template_postgis
psql -l
psql template_postgis -c "CREATE TABLESPACE temp_disk LOCATION '${PGTEMP_DIR}';"
psql template_postgis -c "SET temp_tablespaces TO 'temp_disk';"
psql template_postgis -c "CREATE EXTENSION postgis;"
psql template_postgis -c "SELECT PostGIS_Full_Version();"
curl -OL "https://s3.amazonaws.com/metro-extracts.mapzen.com/seattle_washington.water.coastline.zip"
unzip -o seattle_washington.water.coastline.zip
createdb test-osm -T template_postgis
shp2pgsql -s 4326 seattle_washington_water_coast.shp coast | psql test-osm
psql test-osm -c "SELECT count(*) from coast;"