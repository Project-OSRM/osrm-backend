# OSRM with Telenav Traffic Design

## Architecture
![osrm-with-telenav-traffic-architecture](./graph/osrm-with-telenav-traffic-architecture.mmd.png)

### OSRMTrafficUpdater
- as `RPC` client
- connect `TrafficProxy` by `RPC`
- convert contents from `RPC` protocol to `OSRM` required `csv` format, then write to file

### TrafficProxy
- as `RPC` server
- provide traffic contents by region
- contents include at least `+/-wayid, speed`

## OSRM with Traffic Startup Flow
![osrm-with-traffic-startup-flow-chart](./graph/osrm-with-traffic-startup-flow-chart.mmd.png)

## Release and Deployment Pipeline
![osrm-release-deployment-pipeline](./graph/osrm-release-deployment-pipeline.mmd.png)