# Build OSRM server based on Berlin OSM data

- Generate docker image
```bash
DOCKER_BUILDKIT=1 docker build --no-cache -t telenav/osrm-backend:docker-orchestration-perry --build-arg BRANCH_NAME=feature/docker-orchestration-perry .
```

- Generate OSRM data
```bash
 docker run -d -v /Users/ngxuser/osrm-data/berlin_osm:/osrm-data --name osrm-data telenav/osrm-backend:docker-orchestration-perry compile_mapdata berlin "https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf"
```

- Start OSRM server
```bash
docker run -d -p 5000:5000 -v /Users/ngxuser/osrm-data/berlin_osm:/osrm-data --name osrm-api telenav/osrm-backend:docker-orchestration-perry routed_startup berlin
```
You should see such logs in docker
```
# docker logs -f container_id
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


