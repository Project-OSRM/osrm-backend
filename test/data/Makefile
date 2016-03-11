MONACO_URL:=https://s3.amazonaws.com/mapbox/osrm/testing/monaco.osm.pbf
TOOL_ROOT:=../../build
PROFILE_ROOT:=../../profiles
OSRM_EXTRACT:=$(TOOL_ROOT)/osrm-extract
OSRM_CONTRACT:=$(TOOL_ROOT)/osrm-contract
PROFILE:=$(PROFILE_ROOT)/car.lua

all: monaco.osrm.hsgr

clean:
	rm monaco.*

monaco.osm.pbf:
	wget $(MONACO_URL) -O monaco.osm.pbf

monaco.osrm: monaco.osm.pbf $(PROFILE) $(OSRM_EXTRACT)
	@echo "Verifiyng data file integrity..."
	md5sum -c data.md5sum
	@echo "Running osrm-extract..."
	$(OSRM_EXTRACT) monaco.osm.pbf -p $(PROFILE)

monaco.osrm.hsgr: monaco.osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(OSRM_CONTRACT) monaco.osrm

checksum:
	md5sum monaco.osm.pbf > data.md5sum

.PHONY: clean checksum
