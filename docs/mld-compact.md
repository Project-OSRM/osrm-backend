# Compact MLD query datasets

This opt-in workflow prepares a separate, query-only copy of a fully customized
MLD dataset. It preserves routing and snapping values exactly. The original
prepared dataset remains the input for traffic updates and preprocessing.

```sh
osrm-extract -p profiles/car.lua germany.osm.pbf
osrm-partition germany.osrm
osrm-customize germany.osrm
osrm-compact germany.osrm --output compact-germany/germany.osrm
osrm-routed --algorithm mld compact-germany/germany.osrm
```

The output's parent directory must not exist. Its ancestors must already exist.
`osrm-compact` never modifies input files and copies every file needed for queries;
the result contains no links to the input. An existing output directory is refused,
including an empty directory. Do not update the input while compaction is running.
Parent directory names containing dataset extensions such as `.osrm` are rejected
because the shared OSRM path normalization would reinterpret them.

Compaction failure removes only the directory created by that invocation. A
process interruption can leave a directory containing `.osrm-compact-incomplete`;
OSRM rejects that directory. Prepare a new output directory after an interruption.
All archive writes are closed and checked before the progress marker is removed.
Publication is guarded by that marker; it is not a filesystem transaction or a
power-loss durability guarantee. Deploy complete directories as immutable generations.

## Supported loading and updates

The first release scope is 64-bit little-endian platforms and direct-load MLD.
Loading a compact dataset automatically enables exact cell metric sharing.
The command needs the complete default query dataset, including guidance files.
CH, `--mmap`, memory-file loading, and `osrm-datastore` are rejected for compact
datasets. The source may also have CH files; those files are not copied.

Compact data carries format version 1 inside `.osrm.properties`, in addition to
OSRM's existing platform and version fingerprints. An unknown compact version is
rejected. Keep all files from one generation together. Older OSRM versions are
not supported readers of compact datasets.

Partitioning, contraction, customization, and traffic updates reject compact input
before writing files. Apply updates to the original prepared data, run
`osrm-customize` again, and compact into a new directory. Route services do not
change. To roll back, restart with the original prepared dataset.

## Storage choices

The command canonicalizes complete turn records, including guidance and both
penalties, and remaps graph turn references together. Geometry costs share equal
sequences within each geometry and use exact integer widths. Zero datasource
spans, compact edge-based node metadata, bearing references, graph edges,
partitions, and CSR offsets reduce the remaining storage.

Every fixed-width representation has a checked bound. Inputs that exceed a
compact width retain the original representation; values are never truncated.
Geometry, partition, and CSR encodings also retain the original representation
when their encoded payload would be larger. Nonzero datasource spans remain
unchanged. Cell metrics share only exactly equal matrices at startup.

The current Germany validation reduced warmed resident memory from 5.462 GB to
2.692 GB (50.72%) with one query worker. A motorway-exclusion control saved
50.71%. Startup with prefetched files decreased from 3.115 to 2.099 seconds.
Independent compaction took 78.18 seconds, approximately 3.54% of the earlier
extraction, partitioning and customization time.

The same Dijkstra-rank benchmark measured a 6.34% MLD search-time penalty versus
original storage with cell sharing on both sides (95% source-cluster interval:
6.07% to 6.61%). This is higher than the earlier prototype's 5.30% result.
Excluding version metadata from query allocation did not establish a meaningful
speed improvement. Performance approval remains a release gate. Full Route API
measurements varied substantially between process observations; consult the
experiment report instead of treating one API mean as a guaranteed overhead.

These are dataset and workload measurements, not guarantees for other datasets,
exclusions, worker counts, or query mixes. Production rollout requires validation
on the intended hardware and workload. The spatial-index experiment is separate
from this format. No release has been published from this feature branch.

## Validation before rollout

Build and run the `block_delta`, `compact_dataset`, and `compact` repository test
suites. They cover integer widths 0 through 64, partial blocks, version handling,
failure cleanup, ambiguous-path rejection, independent output files, automatic
sharing, and exact responses
for Route, Nearest, Table, Trip, Match, and Tile on Monaco. Keep byte-exact response
comparisons, full data audits, resident memory, startup time, and Dijkstra-rank
query measurements as release gates on Germany and the deployment datasets.

The Dijkstra rank selects targets by the number of nodes settled by a plain
unidirectional Dijkstra search. The timed operation is the production MLD search
between those targets and sources; API latency is measured separately.
