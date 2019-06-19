# telenav osrm-backend docker
Image within built osrm binaries(`osrm-extract/osrm-partition/osrm-customize/...`) and running dependencies. It can be used to **compile data** or **startup routed**.      

## Build Image
Please use below jenkins job to build docker image within osrm binaries from source code.    

- [(Telenav Internal) Jenkins Job - Build_Telenav_OSRM_Backend_Docker](https://shd-routingfp-01.telenav.cn:8443/view/OSRM/job/Build_Telenav_OSRM_Backend_Docker/)    

## Compile Data
Please use below jenkins job to compile mapdata from PBF to osrm data.       
There're two options for publish the compiled osrm data:    
- generate new docker image from current one within compile data
- export as `map.tar.gz`    

See below job for details:      
- [(Telenav Internal) Jenkins Job - Compile_Mapdata_In_Telenav_OSRM_Backend_Docker](https://shd-routingfp-01.telenav.cn:8443/view/OSRM/job/Compile_Mapdata_In_Telenav_OSRM_Backend_Docker/)    


## Run 
### Run with osrm-data inside

```bash
$ docker pull artifactory.telenav.com/telenav-docker-preprod/osrm-backend:8__9__master-telenav__20190618T135121CST__20190514T140904CST-california-latest
$ 
# run with default trafficproxy
$ docker run -d -p 5000:5000 artifactory.telenav.com/telenav-docker-preprod/osrm-backend:8__9__master-telenav__20190618T135121CST__20190514T140904CST-california-latest routed_startup 

# run with another trafficproxy(e.g. 10.189.102.81)
$ docker run -d -p 5000:5000 artifactory.telenav.com/telenav-docker-preprod/osrm-backend:8__9__master-telenav__20190618T135121CST__20190514T140904CST-california-latest routed_startup 10.189.102.81
```

### Run with osrm-data outside

```bash
# prepare compiled data first (sample data: california)
$ cd osrm-data
$ ll -lh
total 243M
-rw-r--r-- 1 mapuser appuser  21M Jun 17 23:37 map.osrm.cell_metrics
-rw-r--r-- 1 mapuser appuser 193K Jun 17 23:34 map.osrm.cells
-rw-r--r-- 1 mapuser appuser 1.9M Jun 17 23:34 map.osrm.cnbg
-rw-r--r-- 1 mapuser appuser 1.9M Jun 17 23:34 map.osrm.cnbg_to_ebg
-rw-r--r-- 1 mapuser appuser  68K Jun 17 23:37 map.osrm.datasource_names
-rw-r--r-- 1 mapuser appuser 9.8M Jun 17 23:34 map.osrm.ebg
-rw-r--r-- 1 mapuser appuser 2.8M Jun 17 23:34 map.osrm.ebg_nodes
-rw-r--r-- 1 mapuser appuser 2.9M Jun 17 23:34 map.osrm.edges
-rw-r--r-- 1 mapuser appuser 2.7M Jun 17 23:34 map.osrm.enw
-rwx------ 1 mapuser appuser 5.6M Jun 17 23:34 map.osrm.fileIndex
-rw-r--r-- 1 mapuser appuser 7.3M Jun 17 23:37 map.osrm.geometry
-rw-r--r-- 1 mapuser appuser 1.1M Jun 17 23:34 map.osrm.icd
-rw-r--r-- 1 mapuser appuser 5.0K Jun 17 23:34 map.osrm.maneuver_overrides
-rw-r--r-- 1 mapuser appuser  11M Jun 17 23:37 map.osrm.mldgr
-rw-r--r-- 1 mapuser appuser 218K Jun 17 23:34 map.osrm.names
-rw-r--r-- 1 mapuser appuser 4.0M Jun 17 23:34 map.osrm.nbg_nodes
-rw-r--r-- 1 mapuser appuser 1.8M Jun 17 23:34 map.osrm.partition
-rw-r--r-- 1 mapuser appuser 6.0K Jun 17 23:34 map.osrm.properties
-rw-r--r-- 1 mapuser appuser  29K Jun 17 23:34 map.osrm.ramIndex
-rw-r--r-- 1 mapuser appuser 4.0K Jun 17 23:34 map.osrm.restrictions
-rw-r--r-- 1 mapuser appuser 3.5K Jun 17 23:34 map.osrm.timestamp
-rw-r--r-- 1 mapuser appuser 5.5K Jun 17 23:34 map.osrm.tld
-rw-r--r-- 1 mapuser appuser 8.0K Jun 17 23:34 map.osrm.tls
-rw-r--r-- 1 mapuser appuser 836K Jun 17 23:34 map.osrm.turn_duration_penalties
-rw-r--r-- 1 mapuser appuser 4.9M Jun 17 23:34 map.osrm.turn_penalties_index
-rw-r--r-- 1 mapuser appuser 836K Jun 17 23:34 map.osrm.turn_weight_penalties
$ cd ..

# pull & run
$ docker pull wangyoucao577/osrm-backend:9__master-telenav__20190618T135121CST
$ docker run -d -p 5000:5000  "src=$(pwd)/osrm-data,dst=/osrm-data,type=bind" wangyoucao577/osrm-backend:9__master-telenav__20190618T135121CST routed_startup 
```

## Example By Manual
- [Build Berlin Server with OSM data](./example-berlin-osm.md)

