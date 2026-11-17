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

## Release candidate checks, 17 November 2026

Compact output is not ready to become the default. Verification of the unchanged
candidate on Germany preserved all measured outputs, but shared-library error
handling and installed C++ preprocessing headers still have release blockers.
Linux, Windows, ARM, packaged wheels, and default LTO builds remain unverified.

Three fresh process pairs measured 5.469 GB versus 2.699 GB warmed RSS (50.65%
saved), prefetched startup of 3.559 versus 2.267 seconds, and a 2.36% Route API
latency increase. Three repetitions of the unchanged Dijkstra-rank benchmark
verified 288 reference weights and 147,456 timed searches per process. Their
search slowdown was 5.30%, with a hierarchical process/source bootstrap interval
of 4.50% to 6.00%. Individual processes measured 4.56%, 5.97%, and 5.38%; this
is a fresh observation, not a source optimization over the earlier result.
Both sides of the rank comparison share exact cell metrics.

An additional mixed-workload test held all worker caches resident. Two process
pairs per worker count used 80% Route, 10% Table, and 10% Nearest requests:

| Workers | Memory saved | Throughput change | p99 latency change |
| --- | ---: | ---: | ---: |
| 1 | 50.89% | -0.57% | +1.37% |
| 4 | 50.30% | -3.44% | +5.72% |
| 8 | 49.64% | -4.94% | +6.02% |

At eight workers, another 20.0 MB would be needed merely to reach 50% savings in
this workload. These throughput and tail measurements are descriptive, not
deployment guarantees. The host was an Intel i7-7700K with four physical cores,
eight logical threads, and 32 GiB RAM; builds used Apple Clang 17, Release, with
LTO disabled.

The six standard Cucumber configurations completed with 8,937 scenarios passed,
35 skipped, and four failures. All four repeat the existing
`features/options/datastore/datastore.feature:7` assertion expecting a stderr
fragment when no options are supplied; the original main-build binary reproduces
it. A further MLD direct-load pass queried 1,433 independent compact generations
and passed 1,495 scenarios, with that same single CLI assertion failure. No
feature expectations were weakened. Native Python service comparisons across
car, bicycle, and foot also matched exactly, as did nonzero traffic and a valid
full-width annotation fallback. Interrupted-generation and unsupported-mode
checks rejected unsafe loads. Protected Germany files and the user's spatial
index source changes were verified unchanged.

Shared builds failed four compactor exception cases and one customization
preflight case: expected OSRM exceptions escaped their typed catches. Duplicate
exception type information in `libosrm` and `libosrm_store` is the leading
diagnosis. An installed C++ customization consumer also cannot find
`customizer/customizer_config.hpp`. Resolve these issues and finish deployment
and platform validation before changing the default.

Preprocessing was not re-timed in this release check. The earlier 78.34-second
integrated result was 1.32% above the two-command compact workflow, but ordinary
customization alone took 48.31 seconds. Compact creation added about 30.03 seconds
(62.15%) to that stage; the full extract/partition pipeline was not re-timed.
