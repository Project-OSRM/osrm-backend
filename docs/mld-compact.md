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

The initial integrated Germany validation reduced warmed resident memory from 5.462 GB
to 2.697 GB (50.62%) with one query worker. A motorway-exclusion control saved
50.12%. Startup with prefetched files decreased from 3.149 to 2.126 seconds.
With six customization workers, customization plus compact creation took 78.34
seconds versus 77.32 seconds for the two-command control (+1.32%). Peak
preprocessing memory was 7.56 GB versus 7.16 GB (+5.64%). The compact stage uses
sequential, stage-local archive buffers; no routing instance is loaded.

The initial frozen Dijkstra-rank benchmark measured no additional search slowdown from
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

## Earlier local release checks, 18 November 2026

The following snapshot predates platform CI and the STL revision below. The local
failures from the 17 November check were resolved. Compact output remained opt-in,
with platform, packaging, default LTO and deployment validation still open. No
release was published and the default did not change.

Shared builds now use one `osrm_exception` library for OSRM exception definitions.
All 133 shared library cases (277,331 assertions) and six shared customization
cases (321 assertions) pass, including the five previously failing typed catches.
Customizer and updater configuration headers are installed. A new CTest compiles
a consumer from staged public headers; an installed shared C++ consumer also
compiles, links, catches an OSRM exception and queries Monaco. Installed shared
tools run using the default loader-relative install RPATH. Install the exception
library alongside the other OSRM shared libraries.

The final native CTest run passes all 30 build/test checks. All 190 utility cases
(456,123 assertions), 53 CLI parsing checks, and an ASan/UBSan randomized
worker-index probe pass. Native Python matches 18 service comparisons across
car, bicycle and foot and rejects three unsupported compact loading modes.

The complete six-way Cucumber matrix passes 8,941 scenarios with 35 skipped
and zero failures. An additional MLD direct-load pass creates 1,433
independent compact query generations and passes all 1,496 scenarios. The
no-argument datastore assertion now checks the established behavior: help on
stdout, empty stderr and success exit. The command helper now converts an empty
option string to an empty argument list; it previously passed an empty filename.
The complete four-scenario datastore feature also passes separately.
No routing or snapping expectation changed.
The normal `@stress` and `@todo` exclusions remain.

### Worker memory and query cost

MLD query, table and map-matching worker indices now use 16-bit common values,
exact full-width overflow and a dense full-width fallback. The snapshot below
used zero-filled narrow-index allocations. This
changes runtime scratch storage for both ordinary and compact MLD; it does not
change the compact disk format or preprocessing algorithm. Large searches can
trigger the full-width fallback and reduce the savings.

Two process pairs per worker count hold all caches resident, using an 80% Route,
10% Table, 10% Nearest corpus. Execution order reverses in the second pair.

| Workers | Ordinary / compact RSS | Memory saved | Throughput change | p99 change |
| --- | ---: | ---: | ---: | ---: |
| 1 | 5.429 / 2.665 GB | 50.91% | -1.65% | +1.26% |
| 4 | 5.470 / 2.698 GB | 50.67% | +0.07% | -2.96% |
| 8 | 5.509 / 2.738 GB | 50.30% | -0.16% | +0.73% |

Every individual process pair exceeds 50% savings. At eight workers, the margin
below half of ordinary RSS is about 16.8 MB; repeat this test on the deployment
workload. Throughput and p99 observations are descriptive, not production guarantees.

Three interleaved before/after process pairs use the unchanged Dijkstra-rank
benchmark. All six processes verify 288 reference weights and 147,456 timed
searches each, with matching checksums. The geometric mean results are:

| Comparison | MLD search time change | 95% process/source interval |
| --- | ---: | ---: |
| Current compact vs previous compact | +1.21% | +0.48% to +1.95% |
| Current compact vs current ordinary | +5.30% | +4.97% to +5.67% |
| Current compact vs previous ordinary | +6.46% | +5.68% to +7.21% |

The worker-memory fix therefore has a small measurable search cost. The same
change on ordinary MLD measures +1.10%. Intervals use 5,000 hierarchical
process/source bootstrap resamples, with three process pairs and 16 sources.
Both formats share exact cell metrics in this rank test. Rank selects targets
using plain forward Dijkstra; the timed operation is production bidirectional
MLD search. Snapping, unpacking, JSON and HTTP are excluded.

A separate fresh Route API control measures 2.807 ms versus
2.871 ms (+2.28%), prefetched startup
of 3.039 versus 2.013 seconds, and warm RSS
of 5.448 versus 2.682 GB
(50.77% saved). It is one process pair with three timed rounds,
so these API timings do not establish a general slowdown bound.

### Exactness, provenance and remaining gates

Germany canonical API outputs match protected references for default, motorway,
toll and ferry configurations; snapping, hints and tile outputs also match.
The complete audit matches all 23,103,429 nodes, 83,604,186 edges, graph boundaries,
turn values and partition levels. Nonzero traffic and full-width annotation
fallback controls remain exact, including their Dijkstra weights. Ten safety
checks reject unsupported modes and incomplete generations. Protected original
datasets and the user's spatial-index source edits are verified unchanged.

The source fixes are commits `c6a643858`, `acdf5d328`, `28e58c663`, `f98d0919c`
and `67a835835` on `codex/mld-compact-release`. Measurements and the updated report are retained
under `measurements/mld-release-fixes-20261118`. The host is an Intel i7-7700K,
four physical cores/eight logical threads, 32 GiB RAM, macOS x86-64, Apple Clang 17,
Release with LTO disabled. Timed jobs run sequentially before integration tests.

Before changing the default, finish platform, wheel and default-LTO validation,
code review, deployment-worker testing and performance acceptance. Compact input
still supports direct-load MLD only; mmap, datastore, CH and mutable traffic
updates require the original prepared generation. The previous randomized
bicycle baseline timeout remains a coverage gap.

Preprocessing was not re-timed in this fix campaign. Historical integrated
Germany compaction took 78.34 seconds versus 77.32 seconds for customization plus
separate compaction (+1.32%). Ordinary customization alone took 48.31 seconds;
compact creation added about 30.03 seconds (+62.15%) to that stage. The full
extract/partition pipeline was not re-timed.

## STL revision, 18 November 2026

Feature code now uses `std::copy`, `std::copy_n`, `std::equal`, and
`std::bit_cast` for memory operations. Cell matrices compare their object bytes,
preserving signed zero and NaN payloads. `AGENTS.md` records this coding rule.

Worker arrays use `std::make_unique_for_overwrite` and a small initialized-block
table. A first write fills its 512-entry block with `std::fill_n`. An unwritten
block reads as zero without accessing uninitialized values. Full-width promotion
reconstructs every slot, including unwritten blocks. Boundary, partial-tail,
promotion, clear/reuse, and ASan/UBSan checks pass.

Eager standard-array initialization was also tested. It saved 50.10% on average
at eight workers, but one process pair fell just below 50%. The retained block
initialization restores the margin:

| Workers | Process pairs | Ordinary / compact RSS | Memory saved | QPS change | p99 change |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 2 | 5.426 / 2.668 GB | 50.82% | -5.91% | +5.81% |
| 4 | 2 | 5.455 / 2.690 GB | 50.69% | +5.36% | -9.44% |
| 8 | 4 | 5.480 / 2.721 GB | 50.35% | -4.14% | +6.23% |

Every individual pair saves at least 50%. The pooled eight-worker margin is about
19.4 MB below half of ordinary RSS. The extra eight-worker pairs were run
because the initial throughput measurements varied. All observations are retained;
these short throughput and p99 measurements do not establish a deployment bound.

Three interleaved process pairs use the same frozen Dijkstra-rank benchmark,
with 16 sources and matching weights and checksums in all 884,736 timed searches.

| Comparison | Search time change | 95% process/source interval |
| --- | ---: | ---: |
| STL compact vs previous compact | +0.96% | +0.62% to +1.25% |
| Current compact vs current ordinary | +5.10% | +4.85% to +5.35% |
| Current compact vs previous ordinary | +6.05% | +5.64% to +6.40% |

A fresh Route API pair measures 2.755 versus 2.845 ms (+3.26%).
Prefetched startup is 2.991 versus 2.265 seconds (-24.27%).
These API values come from one process pair and three timed rounds. Germany
default, motorway, toll and ferry responses, snapping and other service outputs
match protected references. The full graph and partition audit also passes.
Preprocessing was not re-timed for this revision.

Measured engine: `ef68fb6f0`; previous engine: `f98d0919c`. Artifacts and the
unchanged benchmark-source hashes are retained under
`measurements/mld-stl-blocks-20261118`. The same host and workload definitions
as the earlier snapshot apply. For current compiler, sanitizer and packaging
status, see [PR #7782](https://github.com/Project-OSRM/osrm-backend/pull/7782).
Code review, deployment measurements and performance acceptance are still required
before changing the default. Compact output remains opt-in.
