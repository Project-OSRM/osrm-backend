DATA_NAME:=monaco
DATA_URL:=https://s3.amazonaws.com/mapbox/osrm/testing/$(DATA_NAME).osm.pbf
DATA_POLY_URL:=https://s3.amazonaws.com/mapbox/osrm/testing/$(DATA_NAME).poly
TOOL_ROOT:=../../build
PROFILE_ROOT:=../../profiles
SCRIPT_ROOT:=../../scripts
OSRM_EXTRACT:=$(TOOL_ROOT)/osrm-extract
OSRM_CONTRACT:=$(TOOL_ROOT)/osrm-contract
OSRM_ROUTED:=$(TOOL_ROOT)/osrm-routed
POLY2REQ:=$(SCRIPT_ROOT)/poly2req.js
TIMER:=$(SCRIPT_ROOT)/timer.sh
PROFILE:=$(PROFILE_ROOT)/car.lua

all: $(DATA_NAME).osrm.hsgr

clean:
	rm $(DATA_NAME).*

$(DATA_NAME).osm.pbf:
	wget $(DATA_URL) -O $(DATA_NAME).osm.pbf

$(DATA_NAME).poly:
	wget $(DATA_POLY_URL) -O $(DATA_NAME).poly

$(DATA_NAME).osrm: $(DATA_NAME).osm.pbf $(DATA_NAME).poly $(PROFILE) $(OSRM_EXTRACT)
	@echo "Verifiyng data file integrity..."
	md5sum -c data.md5sum
	@echo "Running osrm-extract..."
	$(TIMER) "osrm-extract" $(OSRM_EXTRACT) $(DATA_NAME).osm.pbf -p $(PROFILE)

$(DATA_NAME).osrm.hsgr: $(DATA_NAME).osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(TIMER) "osrm-contract" $(OSRM_CONTRACT) $(DATA_NAME).osrm

$(DATA_NAME).requests: $(DATA_NAME).poly
	$(POLY2REQ) $(DATA_NAME).poly > $(DATA_NAME).requests

osrm-routed.pid: $(DATA_NAME).osrm.hsgr
	@/bin/sh -c '$(OSRM_ROUTED) $(DATA_NAME).osrm& echo "$$!" > osrm-routed.pid'
	sleep 1

benchmark: $(DATA_NAME).requests osrm-routed.pid
	@echo "Running benchmark..."
	$(TIMER) "queries" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@echo "**** timings ***"
	@cat /tmp/osrm.timings
	@echo "****************"

checksum:
	md5sum $(DATA_NAME).osm.pbf $(DATA_NAME).poly > data.md5sum

.PHONY: clean checksum benchmark
