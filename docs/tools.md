# Command-Line Tools

OSRM ships six command-line tools that cover the full data pipeline, from raw
OSM data to a running routing server. All tools share a set of common options
described below, followed by per-tool reference sections.

## Common Options

These flags are accepted by every tool.

| Flag | Short | Description |
|------|-------|-------------|
| `--help` | `-h` | Show the help message and exit. |
| `--version` | `-v` | Show the version string and exit. |
| `--verbosity <level>` | `-l` | Log verbosity: `NONE`, `ERROR`, `WARNING`, `INFO` (default), `DEBUG`. |
| `--list-inputs` | | Print all required and optional input file extensions the tool expects, then exit. Useful for deployment scripts. |
| `--threads <n>` | `-t` | Number of threads to use (default: number of logical CPUs). |

### `--list-inputs`

Prints one line per file in the format `required|optional <extension>`:

```
$ osrm-routed --list-inputs
required .osrm.datasource_names
required .osrm.ebg_nodes
required .osrm.edges
...
optional .osrm.hsgr
optional .osrm.cells
```

Example — collect all files needed to deploy `osrm-routed`:

```bash
BASE=map
for line in $(osrm-routed --list-inputs); do
    echo "$BASE$line"
done
```

---

## osrm-extract

Reads an OSM file and a Lua profile, and produces the intermediate `.osrm.*`
files consumed by the graph-preparation tools.

```
osrm-extract <input.osm/.osm.bz2/.osm.pbf> [options]
```

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--profile <path>` | `-p` | `profiles/car.lua` | Path to the Lua routing profile. |
| `--output <path>` | `-o` | Derived from input filename | Base path for generated output files. |
| `--data_version <string>` | `-d` | _(none)_ | Tag the dataset with a version string. Use `osmosis` to read the timestamp embedded in the PBF file. |
| `--small-component-size <n>` | | `1000` | Minimum node count for a strongly-connected component to be treated as "large". Affects nearest-neighbor snapping. |
| `--with-osm-metadata` | | | Parse OSM metadata (user, timestamp, etc.). May reduce extraction performance. |
| `--parse-conditional-restrictions` | | | Save conditional turn restrictions to disk so they can be evaluated during contraction. |
| `--location-dependent-data <file>` | | | GeoJSON files containing location-dependent data (e.g. speed limits by region). Repeatable. |
| `--disable-location-cache` | | | Disable the internal node-location cache used for location-dependent data lookups. |
| `--dump-nbg-graph` | | | Write the raw node-based graph to the `.osrm` file for debugging. |

---

## osrm-partition

Partitions the road network graph into a hierarchy of cells used by the
Multi-Level Dijkstra (MLD) algorithm.

```
osrm-partition <input.osrm> [options]
```

| Flag | Default | Description |
|------|---------|-------------|
| `--max-cell-sizes <list>` | `128,4096,65536,2097152` | Comma-separated maximum cell sizes per level, starting from level 1. The first value is also the bisection termination threshold. |
| `--balance <factor>` | `1.2` | Maximum allowed size ratio between the two sides of a single bisection. |
| `--boundary <fraction>` | `0.25` | Fraction of nodes to use as boundary sources/sinks during contraction. |
| `--optimizing-cuts <n>` | `10` | Number of candidate cuts evaluated when optimizing a single bisection. |
| `--small-component-size <n>` | `1000` | Node-count threshold below which a component is treated as small. |

---

## osrm-customize

Applies live traffic data (speed and turn-penalty files) to a partitioned MLD
graph. Can be run repeatedly without re-partitioning when speeds change.

```
osrm-customize <input.osrm> [options]
```

| Flag | Default | Description |
|------|---------|-------------|
| `--segment-speed-file <file>` | | CSV with `nodeA,nodeB,speed` columns to override edge weights. Repeatable. |
| `--turn-penalty-file <file>` | | CSV with `from_node,via_node,to_node,penalty` to override turn weights. Repeatable. |
| `--edge-weight-updates-over-factor <x>` | `0` (disabled) | Log edges whose weight changed by more than factor `x` (requires `--segment-speed-file`). |
| `--parse-conditionals-from-now <utc_timestamp>` | `0` (disabled) | UTC Unix timestamp from which to evaluate conditional turn restrictions. |
| `--time-zone-file <file>` | | GeoJSON file with time-zone boundaries, required for conditional restriction parsing. |

---

## osrm-contract

Builds a Contraction Hierarchy (CH) from the extracted graph. Use this instead
of `osrm-partition` + `osrm-customize` when you don't need live traffic updates.

```
osrm-contract <input.osrm> [options]
```

| Flag | Default | Description |
|------|---------|-------------|
| `--segment-speed-file <file>` | | CSV with `nodeA,nodeB,speed` columns to override edge weights. Repeatable. |
| `--turn-penalty-file <file>` | | CSV with `from_node,via_node,to_node,penalty` to override turn weights. Repeatable. |
| `--edge-weight-updates-over-factor <x>` | `0` (disabled) | Log edges whose weight changed by more than factor `x`. |
| `--parse-conditionals-from-now <utc_timestamp>` | `0` (disabled) | UTC Unix timestamp for evaluating conditional turn restrictions. |
| `--time-zone-file <file>` | | GeoJSON file with time-zone boundaries, required for conditional restriction parsing. |

---

## osrm-routed

The HTTP server. Loads a prepared dataset and serves the OSRM HTTP API.

```
osrm-routed <base.osrm> [options]
```

### Server

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--ip <address>` | `-i` | `0.0.0.0` | IP address to listen on. |
| `--port <n>` | `-p` | `5000` | TCP port to listen on. |
| `--keepalive-timeout <s>` | `-k` | `5` | HTTP keep-alive timeout in seconds. |
| `--trial` | | | Start up fully, then exit immediately. Useful to validate a dataset without serving traffic. |

### Data loading

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--algorithm <name>` | `-a` | `CH` | Routing algorithm: `CH` (Contraction Hierarchy) or `MLD` (Multi-Level Dijkstra). |
| `--shared-memory` | `-s` | off | Load data from a shared memory region managed by `osrm-datastore`. |
| `--mmap` | `-m` | off | Memory-map the data files instead of loading them into RAM. |
| `--dataset-name <name>` | | | Shared memory dataset name to connect to (used with `--shared-memory`). |
| `--disable-feature-dataset <name>` | | | Skip loading an optional dataset to save memory. Options: `ROUTE_STEPS`, `ROUTE_GEOMETRY`. |

### Query limits

| Flag | Default | Description |
|------|---------|-------------|
| `--max-viaroute-size <n>` | `500` | Maximum number of waypoints in a route query. |
| `--max-trip-size <n>` | `100` | Maximum number of locations in a trip query. |
| `--max-table-size <n>` | `100` | Maximum number of locations in a table query. |
| `--max-matching-size <n>` | `100` | Maximum number of locations in a map-matching query. |
| `--max-nearest-size <n>` | `100` | Maximum number of results in a nearest query. |
| `--max-alternatives <n>` | `3` | Maximum number of alternative routes (MLD only). |
| `--max-matching-radius <m>` | `-1` (unlimited) | Maximum search radius in metres for map-matching. |
| `--default-radius <m>` | `-1` (unlimited) | Default snap radius for all queries. |
| `--max-header-size <bytes>` | `0` (auto) | Maximum HTTP header size in bytes. |

---

## osrm-datastore

Loads a prepared dataset into shared memory so that one or more `osrm-routed`
processes can serve it with zero-copy access. Enables live traffic updates
without restarting the server.

```
osrm-datastore [options] <base.osrm>
```

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--dataset-name <name>` | | | Name for this dataset in shared memory. Allows multiple datasets to coexist. |
| `--max-wait <s>` | `-1` (unlimited) | Seconds to wait for a running update to finish before forcibly acquiring the lock. |
| `--only-metric` | | | Reload only the metric (weights/durations) without replacing the full dataset. Optimized for frequent traffic updates. |
| `--disable-feature-dataset <name>` | | | Skip loading an optional dataset. Options: `ROUTE_STEPS`, `ROUTE_GEOMETRY`. |
| `--remove-locks` | `-r` | | Remove stale shared-memory locks and exit. |
| `--spring-clean` | `-s` | | Remove all OSRM shared memory regions and exit. |
| `--list` | | | List all datasets currently loaded in shared memory. |
| `--list-blocks` | | | List all shared memory blocks currently in use. |
