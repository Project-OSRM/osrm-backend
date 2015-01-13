#!/bin/sh

# helper script needed by cmake to run test

TESTDATA_DIR=$1

rm -f multipolygon.db multipolygon-tests.json
./testdata-multipolygon ${TESTDATA_DIR}/grid/data/all.osm >multipolygon.log 2>&1 &&
    ${TESTDATA_DIR}/bin/compare-areas.rb ${TESTDATA_DIR}/grid/data/tests.json multipolygon-tests.json

