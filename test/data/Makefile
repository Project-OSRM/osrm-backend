# Builds the monaco and berlin datasets into
#
# - test/data/ch
# - test/data/mld
# - test/data/tmp  # used as scratch
#
# Note: the files test/data/*.orsm.* may also come from library-extract-tests.

DATASETS = monaco berlin

all: monaco

monaco: verify  # verify first, extract later

OSRM_EXTRACT:=$(OSRM_BUILD_DIR)/osrm-extract$(EXE)
OSRM_CONTRACT:=$(OSRM_BUILD_DIR)/osrm-contract$(EXE)
OSRM_PARTITION:=$(OSRM_BUILD_DIR)/osrm-partition$(EXE)
OSRM_CUSTOMIZE:=$(OSRM_BUILD_DIR)/osrm-customize$(EXE)
OSRM_ROUTED:=$(OSRM_BUILD_DIR)/osrm-routed$(EXE)
MD5SUM:=$(PROJECT_SOURCE_DIR)/scripts/md5sum.js
PROFILE:=$(PROJECT_SOURCE_DIR)/profiles/car.lua

tmp/%.osrm.edges: %.osm.pbf $(PROFILE) $(OSRM_EXTRACT)
	mkdir -p tmp
	cp $< tmp/
	@echo "Running osrm-extraction ..."
	cd tmp && $(OSRM_EXTRACT) $< -p $(PROFILE) > /dev/null

ch/%.osrm.hsgr: tmp/%.osrm.edges $(OSRM_CONTRACT)
	mkdir -p ch
	cp tmp/$*.osrm.* ch/
	@echo "Running osrm-contract ..."
	cd ch && $(OSRM_CONTRACT) $*.osrm > /dev/null

mld/%.osrm.mldgr: tmp/%.osrm.edges $(OSRM_PARTITION) $(OSRM_CUSTOMIZE)
	mkdir -p mld
	cp tmp/$*.osrm.* mld/
	@echo "Running osrm-partition ..."
	cd mld && $(OSRM_PARTITION) $*.osrm > /dev/null
	@echo "Running osrm-customize ..."
	cd mld && $(OSRM_CUSTOMIZE) $*.osrm > /dev/null

define OSRM_template =

$(1): ch/$(1).osrm.hsgr mld/$(1).osrm.mldgr

.PRECIOUS: tmp/$(1).osrm.edges

clean-$(1):
	-rm -rf ch/$(1).* mld/$(1).* tmp/$(1).*

clean: clean-$(1)

endef

$(foreach ds,$(DATASETS),$(eval $(call OSRM_template,$(ds))))

berlin.osm.pbf:
	curl https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf -o $@ --location --silent

%.md5sum: %.osm.pbf
	$(MD5SUM) $< > $@

clean:
	-rm -rf *.osrm.*

checksum: monaco.md5sum

verify:
	@echo -n "Verifying data file integrity "
	@$(MD5SUM) -c monaco.md5sum

.PHONY: all clean checksum verify $(DATASETS)
