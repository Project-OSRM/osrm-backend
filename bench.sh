
REGION=poland-latest

# mkdir -p $REGION
# cd $REGION
# wget http://download.geofabrik.de/europe/$REGION.osm.pbf
# ../build/osrm-extract --profile ../profiles/car.lua $REGION.osm.pbf
# ../build/osrm-partition ./$REGION
# ../build/osrm-customize ./$REGION
# cd ..


node test/nodejs/benchmark.js $REGION/$REGION.osrm "18.638306,54.372158;19.944544,50.049683"