DATA_NAME:=monaco
OSRM_BUILD_DIR?=../../build
PROFILE_ROOT:=../../profiles
SCRIPT_ROOT:=../../scripts
OSRM_EXTRACT:=$(OSRM_BUILD_DIR)/osrm-extract
OSRM_CONTRACT:=$(OSRM_BUILD_DIR)/osrm-contract
OSRM_PARTITION:=$(OSRM_BUILD_DIR)/osrm-partition
OSRM_CUSTOMIZE:=$(OSRM_BUILD_DIR)/osrm-customize
OSRM_ROUTED:=$(OSRM_BUILD_DIR)/osrm-routed
POLY2REQ:=$(SCRIPT_ROOT)/poly2req.js
MD5SUM:=$(SCRIPT_ROOT)/md5sum.js
TIMER:=$(SCRIPT_ROOT)/timer.js
PROFILE:=$(PROFILE_ROOT)/car.lua

all: data

data: ch/$(DATA_NAME).osrm.hsgr mld/$(DATA_NAME).osrm.partition

clean:
	-rm -r $(DATA_NAME).*
	-rm -r ch mld

ch/$(DATA_NAME).osrm: $(DATA_NAME).osrm
	mkdir -p ch
	cp $(DATA_NAME).osrm.* ch/

mld/$(DATA_NAME).osrm: $(DATA_NAME).osrm
	mkdir -p mld
	cp $(DATA_NAME).osrm.* mld/

$(DATA_NAME).osrm: $(DATA_NAME).osm.pbf $(DATA_NAME).poly $(PROFILE) $(OSRM_EXTRACT)
	@echo "Verifying data file integrity..."
	$(MD5SUM) -c data.md5sum
	@echo "Running osrm-extract..."
	$(TIMER) "osrm-extract\t$@" $(OSRM_EXTRACT) $< -p $(PROFILE)

ch/$(DATA_NAME).osrm.hsgr: ch/$(DATA_NAME).osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(TIMER) "osrm-contract\t$@" $(OSRM_CONTRACT) $<

mld/$(DATA_NAME).osrm.partition: mld/$(DATA_NAME).osrm $(PROFILE) $(OSRM_PARTITION)
	@echo "Running osrm-partition..."
	$(TIMER) "osrm-partition\t$@" $(OSRM_PARTITION) $<
	$(TIMER) "osrm-customize\t$@" $(OSRM_CUSTOMIZE) $<

$(DATA_NAME).requests: $(DATA_NAME).poly
	$(POLY2REQ) $(DATA_NAME).poly > $(DATA_NAME).requests

benchmark: data $(DATA_NAME).requests
	@echo "Running benchmark..."
	@/bin/sh -c '$(OSRM_ROUTED) --algorithm=CH ch/$(DATA_NAME).osrm > /dev/null & echo "$$!" > osrm-routed.pid'
	@sleep 1
	$(TIMER) "queries\tCH" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@/bin/sh -c '$(OSRM_ROUTED) --algorithm=MLD mld/$(DATA_NAME).osrm > /dev/null & echo "$$!" > osrm-routed.pid'
	@sleep 1
	$(TIMER) "queries\tMLD" "cat $(DATA_NAME).requests | xargs curl &> /dev/null"
	@cat osrm-routed.pid | xargs kill
	@rm osrm-routed.pid
	@echo "**** timings ***"
	@cat /tmp/osrm.timings
	@echo "****************"

checksum:
	$(MD5SUM) $(DATA_NAME).osm.pbf $(DATA_NAME).poly > data.md5sum

.PHONY: clean checksum benchmark data
