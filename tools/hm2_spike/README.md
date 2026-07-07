# HM2 `/tree` plugin — tooling

## `regression.py` — the committed regression harness (run this)

Table-driven suite that locks in the known-good NL behaviour of the `/tree` plugin. It runs
against a live `osrm-routed` and **exits nonzero on any regression**.

```
OSRM_URL=http://127.0.0.1:5050 python3 regression.py     # local demo server
OSRM_URL=http://osrm-routed:5000 python3 regression.py   # in-cluster
python3 regression.py [base_url]                          # positional override
```

**Run it after every rebase of the fork and after every dataset (Geofabrik) refresh** — a
rebase can break the OSRM-internals coupling and a data refresh can move a snap or renumber a
ref, neither of which the build catches. Green (exit 0) is the ship gate. See
[`REBASING.md`](../../REBASING.md) for the full procedure.

Covers: A2→A27→A15 (Everdingen, Deil), A4→A9 (Badhoevedorp), A15→A16 fork (Ridderkerk);
afritten (N320/N327/N201/N207/N11) never qualify or branch; A2/A67 concurrency has zero
toward-self junctions to 200 km; A10 ring terminates under a 50 km cap; off-motorway →
`400 NoSegment`; every junction name is `""`.

## `probe.py` / `walk_probe.py` / `tree_probe.py` — spike-era probes (kept for evidence)

The Stage 1/2/3 ad-hoc probes that produced `evidence/*.json` during the spike. Superseded by
`regression.py` for CI-style checking; retained because their raw dumps are the evidence behind
`SPIKE-REPORT.md`. `probe.py` also documents the corridor method used to derive the pinned
probe coordinates.
