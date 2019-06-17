# OSRM with Telenav Traffic Design (Draft)

## Architecture
![osrm-with-telenav-traffic-architecture](./graph/osrm-with-telenav-traffic-architecture.mmd.png)

### OSRMTrafficUpdater(need to implement)
- as client
- connect `TrafficProxy` by `RPC`
- convert contents from `RPC` protocol to `OSRM` required `csv` format, then write to file

### TrafficProxy(need to implement)
- as server
- provide traffic contents by region
- contents include at least `from node, to node, speed`(both `from node` and `to node` are come from original mapdata)
- known issues/questions: 
    - can not compile `OSM` mapdata to traffic graph?    


## OSRM with Traffic Startup Flow
![osrm-with-traffic-startup-flow-chart](./graph/osrm-with-traffic-startup-flow-chart.mmd.png)

## Release and Deployment Pipeline
![osrm-release-deployment-pipeline](./graph/osrm-release-deployment-pipeline.mmd.png)