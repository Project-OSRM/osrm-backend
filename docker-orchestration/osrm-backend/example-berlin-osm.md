# Build OSRM server based on Berlin OSM data

- Generate docker image
```bash
$ DOCKER_BUILDKIT=1 docker build --no-cache -t telenav/osrm-backend:master-telenav --build-arg BRANCH_NAME=master-telenav --build-arg IMAGE_TAG=master-telenav-20190620 .
```

- Generate OSRM data
```bash
$ mkdir -p compiled-data
$ docker run -it --mount "src=$(pwd)/compiled-data,dst=/save-data,type=bind" telenav/osrm-backend:master-telenav compile_mapdata "https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf" false true
$ ll compiled-data/                                                                                  
total 34968
-rw-r--r-- 1 root root 35805710 Jun 17 23:34 map.tar.gz
```

- Start OSRM server
```bash
$ cd compiled-data
$ tar -zxf map.tar.gz
$ docker run -d -p 5000:5000 --mount "src=$(pwd),dst=/osrm-data,type=bind" --name osrm-api telenav/osrm-backend:master-telenav routed_startup
05227a108a66e7c59f7515f8f174a65f9932f36fe3807f83991806c0194a7e50
```
You should see such logs in docker
```
$ docker logs 05227a108a66e7c59f7515f8f174a65f9932f36fe3807f83991806c0194a7e50
[info] starting up engines, v5.22.0
[info] Threads: 10
[info] IP address: 0.0.0.0
[info] IP port: 5000
[info] http 1.1 compression handled by zlib version 1.2.8
[info] Listening on: 0.0.0.0:5000
[info] running and waiting for requests
```

- Test
```bash
curl "http://127.0.0.1:5000/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219"
```


