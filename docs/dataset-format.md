# Routing dataset format

The preprocessing tools write the `.osrm.*` files that OSRM uses for routing.
Direct loading, `--mmap`, and `osrm-datastore` all use the same dataset format.

OSRM packs integer fields and stores identical turn records once per routing
graph. For each MLD cell, customization stores one copy of each distinct matrix
across the configured profile exclusions. These structures preserve exact values.
Integer fields use wider storage when their values require it.

`osrm-customize` updates MLD weights and durations using the original turn
identities. Run `osrm-datastore --only-metric` to reload updated metrics for a
dataset served from shared memory. A dataset can contain both CH and MLD graphs.
Rebuild both graphs after extraction or partitioning, and run preprocessing
stages in sequence.

Rebuild legacy query-only exports from OSM before applying traffic updates.
For preprocessing commands and options, see [command-line tools](tools.md).
