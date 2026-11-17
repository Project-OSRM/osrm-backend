# Compact MLD query datasets

This opt-in workflow creates a compact, query-only generation during MLD
customization. Non-compact preprocessing remains the default. It preserves routing and snapping values exactly. The original
prepared dataset remains the input for traffic updates and preprocessing.

```sh
osrm-extract -p profiles/car.lua germany.osm.pbf
osrm-partition germany.osrm
osrm-customize germany.osrm --compact-output compact-germany/germany.osrm
osrm-routed --algorithm mld compact-germany/germany.osrm
```

The output's parent directory must not exist. Its ancestors must already exist.
`osrm-customize` updates the normal prepared data as usual, then creates the
compact generation. `--compact-output` is separate from `--output`: the latter
redirects the ordinary MLD graph and metrics, and the compact stage automatically
combines them with the common input files. No separate conversion command is
required. All files needed for queries are copied into the compact generation;
the result contains no links to the input. An existing output directory is refused,
including an empty directory. Destination preflight runs before customization or
traffic updates. A failure during the final compact stage leaves the normal
customized data available and returns a failure status. Do not update the input
concurrently. Customization working arrays are released before the compact stage.
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

The standalone `osrm-compact input.osrm --output new-directory/dataset.osrm`
command remains available for an already customized dataset. It uses the same
writer and does not modify its input. In the C++ preprocessing API, set
`CustomizationConfig::CompactOutputPath` to request the integrated stage.

Compact data carries format version 1 inside `.osrm.properties`, in addition to
OSRM's existing platform and version fingerprints. An unknown compact version is
rejected. Keep all files from one generation together. Older OSRM versions are
not supported readers of compact datasets.

Partitioning, contraction, customization, and traffic updates reject compact input
before writing files. Apply updates to the original prepared data, run
`osrm-customize --compact-output` again with a new output directory. Route services do not
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

The integrated Germany validation reduced warmed resident memory from 5.462 GB
to 2.697 GB (50.62%) with one query worker. A motorway-exclusion control saved
50.12%. Startup with prefetched files decreased from 3.149 to 2.126 seconds.
With six customization workers, customization plus compact creation took 78.34
seconds versus 77.32 seconds for the two-command control (+1.32%). Peak
preprocessing memory was 7.56 GB versus 7.16 GB (+5.64%). The compact stage uses
sequential, stage-local archive buffers; no routing instance is loaded.

The frozen Dijkstra-rank benchmark measured no additional search slowdown from
integration: -0.20% versus standalone compact output (95% source-cluster interval:
-0.30% to -0.10%). The compact format's search cost versus raw storage was +6.82%
(6.47% to 7.16%), with exact cell sharing on both sides. That representation cost
remains a release performance gate. API timings are descriptive observations,
not tail-latency guarantees. The retained Germany output matched the standalone
control byte-for-byte across all query archive entries and spatial-index leaves.

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
