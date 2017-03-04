DATA_NAME:=monaco
DATA_URL:=https://s3.amazonaws.com/mapbox/osrm/testing/$(DATA_NAME).osm.pbf
DATA_POLY_URL:=https://s3.amazonaws.com/mapbox/osrm/testing/$(DATA_NAME).poly
OSRM_BUILD_DIR?=../../build
PROFILE_ROOT:=../../profiles
SCRIPT_ROOT:=../../scripts
OSRM_EXTRACT:=$(OSRM_BUILD_DIR)/osrm-extract
OSRM_CONTRACT:=$(OSRM_BUILD_DIR)/osrm-contract
OSRM_PARTITION:=$(OSRM_BUILD_DIR)/osrm-partition
OSRM_ROUTED:=$(OSRM_BUILD_DIR)/osrm-routed
POLY2REQ:=$(SCRIPT_ROOT)/poly2req.js
MD5SUM:=$(SCRIPT_ROOT)/md5sum.js
TIMER:=$(SCRIPT_ROOT)/timer.sh
PROFILE:=$(PROFILE_ROOT)/car.lua

all: $(DATA_NAME).osrm.hsgr $(DATA_NAME).osrm.partition

clean:
	-rm $(DATA_NAME).*

$(DATA_NAME).osm.pbf:
	wget $(DATA_URL) -O $(DATA_NAME).osm.pbf

$(DATA_NAME).poly:
	wget $(DATA_POLY_URL) -O $(DATA_NAME).poly

$(DATA_NAME).osrm: $(DATA_NAME).osm.pbf $(DATA_NAME).poly $(PROFILE) $(OSRM_EXTRACT)
	@echo "Verifiyng data file integrity..."
	$(MD5SUM) -c data.md5sum
	@echo "Running osrm-extract..."
	$(TIMER) "osrm-extract" $(OSRM_EXTRACT) $(DATA_NAME).osm.pbf -p $(PROFILE)

$(DATA_NAME).osrm.hsgr: $(DATA_NAME).osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(TIMER) "osrm-contract" $(OSRM_CONTRACT) $(DATA_NAME).osrm

$(DATA_NAME).osrm.partition: $(DATA_NAME).osrm $(PROFILE) $(OSRM_PARTITION)
	@echo "Running osrm-partition..."
	$(TIMER) "osrm-partition" $(OSRM_PARTITION) $(DATA_NAME).osrm

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
	$(MD5SUM) $(DATA_NAME).osm.pbf $(DATA_NAME).poly > data.md5sum

.PHONY: clean checksum benchmark
