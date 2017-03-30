DATA_NAME:=monaco
DATA_URL:=https://s3.amazonaws.com/mapbox/osrm/testing/$(DATA_NAME).osm.pbf
DATA_POLY_URL:=https://s3.amazonaws.com/mapbox/osrm/testing/$(DATA_NAME).poly
OSRM_BUILD_DIR?=../../build
PROFILE_ROOT:=../../profiles
SCRIPT_ROOT:=../../scripts
OSRM_EXTRACT:=$(OSRM_BUILD_DIR)/osrm-extract
OSRM_CONTRACT:=$(OSRM_BUILD_DIR)/osrm-contract
OSRM_PARTITION:=$(OSRM_BUILD_DIR)/osrm-partition
OSRM_CUSTOMIZE:=$(OSRM_BUILD_DIR)/osrm-customize
OSRM_ROUTED:=$(OSRM_BUILD_DIR)/osrm-routed
MD5SUM:=$(SCRIPT_ROOT)/md5sum.js
PROFILE:=$(PROFILE_ROOT)/car.lua

all: data

data: ch/$(DATA_NAME).osrm.hsgr corech/$(DATA_NAME).osrm.hsgr mld/$(DATA_NAME).osrm.partition

clean:
	-rm -r $(DATA_NAME).*
	-rm -r ch corech mld

$(DATA_NAME).osm.pbf:
	wget $(DATA_URL) -O $(DATA_NAME).osm.pbf

$(DATA_NAME).poly:
	wget $(DATA_POLY_URL) -O $(DATA_NAME).poly

ch/$(DATA_NAME).osrm: $(DATA_NAME).osrm
	mkdir -p ch
	cp $(DATA_NAME).osrm $(DATA_NAME).osrm.* ch/

corech/$(DATA_NAME).osrm: $(DATA_NAME).osrm
	mkdir -p corech
	cp $(DATA_NAME).osrm $(DATA_NAME).osrm.* corech/

mld/$(DATA_NAME).osrm: $(DATA_NAME).osrm
	mkdir -p mld
	cp $(DATA_NAME).osrm $(DATA_NAME).osrm.* mld/

$(DATA_NAME).osrm: $(DATA_NAME).osm.pbf $(DATA_NAME).poly $(PROFILE) $(OSRM_EXTRACT)
	@echo "Verifiyng data file integrity..."
	$(MD5SUM) -c data.md5sum
	@echo "Running osrm-extract..."
	$(OSRM_EXTRACT) $< -p $(PROFILE)

ch/$(DATA_NAME).osrm.hsgr: ch/$(DATA_NAME).osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(OSRM_CONTRACT) $<

corech/$(DATA_NAME).osrm.hsgr: corech/$(DATA_NAME).osrm $(PROFILE) $(OSRM_CONTRACT)
	@echo "Running osrm-contract..."
	$(OSRM_CONTRACT) --core=0.5 $<

mld/$(DATA_NAME).osrm.partition: mld/$(DATA_NAME).osrm $(PROFILE) $(OSRM_PARTITION)
	@echo "Running osrm-partition..."
	$(OSRM_PARTITION) $<
	$(OSRM_CUSTOMIZE) $<

checksum:
	$(MD5SUM) $(DATA_NAME).osm.pbf $(DATA_NAME).poly > data.md5sum

.PHONY: clean checksum data
