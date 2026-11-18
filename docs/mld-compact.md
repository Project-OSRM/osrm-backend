# Compact dataset storage

Normal preprocessing writes compact records directly into the usual `.osrm.*`
files. There is no conversion command, extra pass, or second query dataset.
Direct loading, `--mmap`, and `osrm-datastore` use the same files.

```sh
osrm-extract -p profiles/car.lua germany.osm.pbf
osrm-partition germany.osrm
osrm-customize germany.osrm
osrm-routed --algorithm mld germany.osrm
```

Records pack integer fields, deduplicate identical turns, and share identical
cell matrices. All values remain exact; fields that exceed a compact width use
a wider encoding.

Traffic updates use the usual customization options. Compact preprocessing
files retain the original turn identities; each routing graph contains its own
query turn dictionary. Repeated customization and datastore metric reloads are
supported. CH and MLD can share a dataset; rebuild both graphs after extraction
or partitioning. Do not run preprocessing stages concurrently on one dataset.

Older prepared datasets remain readable. Rebuild old query-only exports from
OSM before applying traffic updates.
