BERLIN_URL:=https://s3.amazonaws.com/mapbox/node-osrm/testing/berlin-latest.osm.pbf
TOOL_ROOT:=../../build
PROFILE_ROOT:=../../profiles
OSRM_EXTRACT:=$(TOOL_ROOT)/osrm-extract
OSRM_PREPARE:=$(TOOL_ROOT)/osrm-prepare
PROFILE:=$(PROFILE_ROOT)/car.lua

all: berlin-latest.osrm.hsgr

clean:
	rm berlin-latest.*

berlin-latest.osm.pbf:
	wget $(BERLIN_URL) -O berlin-latest.osm.pbf

berlin-latest.osrm: berlin-latest.osm.pbf $(PROFILE) $(OSRM_EXTRACT)
	@echo "Verifiyng data file integrity..."
	md5sum -c data.md5sum
	@echo "Running osrm-extract..."
	$(OSRM_EXTRACT) berlin-latest.osm.pbf -p $(PROFILE)

berlin-latest.osrm.hsgr: berlin-latest.osrm $(PROFILE) $(OSRM_PREPARE)
	@echo "Running osrm-prepare..."
	$(OSRM_PREPARE) berlin-latest.osrm -p $(PROFILE)

checksum:
	md5sum berlin-latest.osm.pbf > data.md5sum

.PHONY: clean checksum
