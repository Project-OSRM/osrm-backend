# OSRM (Highway Mode 2) — Kubernetes manifests (DRAFT)

> **These manifests are DRAFTS. They have NOT been applied and MUST be reviewed by infra
> before use.** They are modelled on the house conventions in the `e-flux` deployment repo
> (`deployment/environments/*`) but make several assumptions that only infra can ratify —
> see "Open questions for infra" below.

## What this deploys

The OSRM fork (the `/tree` plugin, Highway Mode 2) as an in-cluster routing service, plus
the monthly data pipeline that keeps its dataset fresh.

| File | Kind | Purpose |
|---|---|---|
| `osrm-pvc.yaml` | PersistentVolumeClaim | Shared dataset volume (pipeline writes, router reads). |
| `osrm-routed.deployment.yaml` | Deployment | `osrm-routed --algorithm mld` serving `current/data.osrm`. |
| `osrm-routed.service.yaml` | Service (ClusterIP :5000) | In-cluster endpoint for the charger-ahead BE. |
| `osrm-pipeline.cronjob.yaml` | CronJob | Monthly Geofabrik download → extract/partition/customize → swap `current`. |
| `osrm-pipeline.rbac.yaml` | SA + Role + RoleBinding | Lets the pipeline trigger a router rollout after a refresh. |
| `kustomization.yaml` | Kustomization | Bundles the above; image tag placeholder. |

The image is built from `docker/Dockerfile.hm2` (data-less; the four binaries + profiles +
`osrm-pipeline`). Data lives only on the PVC.

## Conventions mirrored from the house repo

- `name: <svc>` labels + `selector.matchLabels.name` (as in `services/api`, `infra/redis`).
- `imagePullSecrets: [artifact-registry]`, image under `europe-west3-docker.pkg.dev/eflux-*/docker/…`, `imagePullPolicy: Always`.
- CronJob shape from `services/platform-usage-collector`: `concurrencyPolicy: Forbid`, `successfulJobsHistoryLimit: 1`, `failedJobsHistoryLimit: 3`, `restartPolicy: OnFailure`.
- PVC shape from `infra/redis` (storageClass + requests), **except** access mode — see below.
- Explicit `resources.requests`/`limits` on every container.

## Open questions for infra (things the house conventions didn't settle)

1. **RWX storage.** The dataset PVC is read by the router and written by the CronJob at the
   same time, so it is declared `ReadWriteMany`. No RWX storage class was found in the
   deployment repo (only `standard-ssd` / RWO). Either provision an RWX class (Filestore/NFS)
   or accept RWO + co-locate CronJob and a single-replica Deployment on one node via
   nodeAffinity. `storageClassName: standard-rwx` in `osrm-pvc.yaml` is a placeholder.
2. **Dataset reload = rollout.** `osrm-routed` doesn't hot-reload a symlink swap. The draft
   triggers a rolling restart from the pipeline (RBAC provided). Confirm this is acceptable,
   or wire a dedicated reconciler. (`osrm-datastore` shared-memory reload is an alternative
   but is more moving parts.)
3. **Namespace.** Not set (a `routing` namespace vs `infra`). The house sets namespace at the
   kustomization/overlay level.
4. **Image build + tag automation.** Hook `osrm-hm2` into the same `image-update-automation`
   used for `services/api`, or pin to a gitsha.
5. **Metrics.** `osrm-routed` exposes no Prometheus metrics natively; the house adds a
   `metrics: enabled` label + `:8082`. If dashboards are required, front it with an exporter
   or the blackbox `Probe` CRD (as `services/api/server/api-probe.yml` does) against `/route`.
6. **Readiness sizing.** Probes call a real `/route` between two Amsterdam points; adjust the
   coordinates if NL is not the first served region.

## Regression harness (run after every rebase / data refresh)

The plugin's behaviour is locked by `tools/hm2_spike/regression.py`. After rebuilding the
fork or refreshing the dataset, run it against the serving instance:

```
OSRM_URL=http://osrm-routed:5000 python3 tools/hm2_spike/regression.py   # in-cluster
OSRM_URL=http://127.0.0.1:5050  python3 tools/hm2_spike/regression.py    # local demo
```

Exit 0 = green; nonzero = a regression (details printed). See `REBASING.md` for the full
fork-maintenance procedure.
