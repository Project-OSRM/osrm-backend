## Abstract

OSRM routed daemon publish health information in prometheus format.
This option switched off by default. To enable this feature please set `-P <monitoring_port>` option on OSRM routed startup.


## Available metrics

- `osrm_routed_instance_info` metric with base instanse information inside labels
-- `algorithm` routing algorithm;
-- `code_version` OSRM version. Same with `-v` command line option;
-- `data_version` OSM data version if available. This value can be setted at the `osrm-extract` phase;
-- `working_threads` actual count of http requests processing threads;
- `http_requests_count` http calls count divided by plugins. And `invalid` count for requests were plugin wasn't set;
- `workers_busy` http workers count that process request at this moment;
