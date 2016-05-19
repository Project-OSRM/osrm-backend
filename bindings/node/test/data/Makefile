
OSRM_RELEASE := $(shell node -e "console.log(require('../../../../package.json').osrm_release)")

ifeq ($(OSRM_RELEASE),)
$(error OSRM_RELEASE variable was not correct set)
endif

BERLIN_URL:=https://s3.amazonaws.com/mapbox/node-osrm/testing/berlin-latest.osm.pbf

all: berlin-latest.osrm.hsgr

clean:
	rm -f ./berlin-latest.*

berlin-latest.osm.pbf:
	wget $(BERLIN_URL) -O berlin-latest.osm.pbf

berlin-latest.osrm: berlin-latest.osm.pbf $(OSRM_EXTRACT)
	@echo "Verifiyng data file integrity..."
	md5sum -c data.md5sum
	@PATH="${PATH}:../../../../build" && echo "*** Using osrm-extract from `which osrm-extract` ***"
	@echo "Running osrm-extract..."
	@PATH="${PATH}:../../../../build" && osrm-extract berlin-latest.osm.pbf -p ../../../../profiles/car.lua

berlin-latest.osrm.hsgr: berlin-latest.osrm $(OSRM_CONTRACT)
	@PATH="${PATH}:../../../../build" && echo "*** Using osrm-contract from `which osrm-contract` ***"
	@echo "Running osrm-contract..."
	@PATH="${PATH}:../../../../build" && osrm-contract berlin-latest.osrm

checksum:
	md5sum berlin-latest.osm.pbf > data.md5sum

.PHONY: clean checksum
