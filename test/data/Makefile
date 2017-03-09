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

all: data

data: $(DATA_NAME)_CH.osrm.hsgr $(DATA_NAME)_CoreCH.osrm.hsgr $(DATA_NAME)_MLD.osrm.partition

clean:
	-rm $(DATA_NAME).*
	-rm $(DATA_NAME)_*

$(DATA_NAME).osm.pbf:
	wget $(DATA_URL) -O $(DATA_NAME).osm.pbf

$(DATA_NAME)_%.osm.pbf: $(DATA_NAME).osm.pbf
	cp $< $@

$(DATA_NAME).poly:
	wget $(DATA_POLY_URL) -O $(DATA_NAME).poly

$(DATA_NAME)_%.osrm: $(DATA_NAME)_%.osm.pbf $(DATA_NAME).poly $(PROFILE) $(OSRM_EXTRACT)
	@echo "Verifiyng data file integrity..."
	$(MD5SUM) -c data.md5sum
	@echo "Running osrm-extract..."
	$(TIMER) "osrm-extract" $(OSRM_EXTRACT) $< -p $(PROFILE)

$(DATA_NAME)_CH.osrm.hsgr: $(DATA_NAME)_CH.osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(TIMER) "osrm-contract" $(OSRM_CONTRACT) $<

$(DATA_NAME)_CoreCH.osrm.hsgr: $(DATA_NAME)_CoreCH.osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(TIMER) "osrm-contract" $(OSRM_CONTRACT) --core=0.95 $<

$(DATA_NAME)_MLD.osrm.partition: $(DATA_NAME)_MLD.osrm $(PROFILE) $(OSRM_PARTITION)
	@echo "Running osrm-partition..."
	$(TIMER) "osrm-contract" $(OSRM_CONTRACT) $<
	$(TIMER) "osrm-partition" $(OSRM_PARTITION) $<

$(DATA_NAME).requests: $(DATA_NAME).poly
	$(POLY2REQ) $(DATA_NAME).poly > $(DATA_NAME).requests

benchmark: data $(DATA_NAME).requests
	@echo "Running benchmark..."
	@/bin/sh -c '$(OSRM_ROUTED) --algorithm=CH $(DATA_NAME)_CH.osrm > /dev/null & echo "$$!" > osrm-routed.pid'
	@sleep 1
	$(TIMER) "queries" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@/bin/sh -c '$(OSRM_ROUTED) --algorithm=CoreCH $(DATA_NAME)_CoreCH.osrm > /dev/null & echo "$$!" > osrm-routed.pid'
	@sleep 1
	$(TIMER) "queries" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@/bin/sh -c '$(OSRM_ROUTED) --algorithm=MLD $(DATA_NAME)_MLD.osrm > /dev/null & echo "$$!" > osrm-routed.pid'
	@sleep 1
	$(TIMER) "queries" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@echo "**** timings ***"
	@cat /tmp/osrm.timings
	@echo "****************"

checksum:
	$(MD5SUM) $(DATA_NAME).osm.pbf $(DATA_NAME).poly > data.md5sum

.PHONY: clean checksum benchmark data
