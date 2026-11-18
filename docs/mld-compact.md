# Compact query data

MLD customization creates compact query data automatically. Use the normal commands:

```sh
osrm-extract -p profiles/car.lua germany.osm.pbf
osrm-partition germany.osrm
osrm-customize germany.osrm
osrm-routed --algorithm mld germany.osrm
```

The generated `germany.osrm.query` archive contains packed graph and geometry
records, deduplicated turns, and exactly shared cell matrices. Direct loading,
`--mmap`, and `osrm-datastore` use it automatically. No compaction switch is needed.
CH and MLD can share the same prepared dataset.

Apply traffic updates as usual:

```sh
osrm-customize germany.osrm --segment-speed-file speeds.csv --turn-penalty-file turns.csv
```

Customization refreshes the query archive while retaining the original
preprocessing files and turn identities. Repeated customization, partitioning,
and contraction remain supported. Datastore reloads publish a consistent set of
graph and guidance data, including when `--only-metric` is requested.

Deploy the `.osrm.query` archive with the other dataset files. Complete archives
are published after writing succeeds; unfinished or stale generations are never
selected. Older datasets without an archive still load through the original
representation. Do not run preprocessing stages concurrently on one dataset.

All encodings preserve exact values and use the original width when a compact
width cannot represent the data. No routing or snapping approximation is used.

For an independent query-only export, `osrm-compact germany.osrm --output
new-directory/germany.osrm` remains available. Its output directory must be new;
retain the prepared source for future traffic updates.
