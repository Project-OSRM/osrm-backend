# Common Tool Options

These options are available for all OSRM command-line tools:
`osrm-extract`, `osrm-partition`, `osrm-customize`, `osrm-contract`, `osrm-routed`, and `osrm-datastore`.

## --list-inputs

Lists all required and optional input file extensions that the tool expects.
Useful for deployment scripts that need to know which files to copy.
Duplicate entries are suppressed when a tool combines multiple configs.

Example:
```
$ osrm-routed --list-inputs
required .osrm.datasource_names
required .osrm.ebg_nodes
required .osrm.edges
...
optional .osrm.hsgr
optional .osrm.cells
```

```
$ osrm-extract --list-inputs
required .osrm.ebg
required .osrm.ebg_nodes
...
```

The output format is one line per file: `required|optional <extension>`.
A deployment script can use this to determine the exact set of files needed:

```bash
BASE=map
for line in $(osrm-routed --list-inputs); do
    echo "$BASE$line"
done
```
