# telenav osrm-backend docker

## Build Image

```bash
$ cd docker-orchestration/osrm-backend

# build source from default branch master-telenav
$ DOCKER_BUILDKIT=1 docker build -t telenav/osrm-backend .

# build source from specified branch, e.g. feature/telenav-import-internal-pbf 
$ DOCKER_BUILDKIT=1 docker build -t telenav/osrm-backend:telenav-import-internal-pbf --build-arg BRANCH_NAME=feature/telenav-import-internal-pbf .

```

## Example
- [Build Berlin Server with OSM data](./example-berlin-osm.md)

