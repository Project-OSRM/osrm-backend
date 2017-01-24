## Open Source Routing Machine

| Linux / macOS | Windows | Code Coverage |
| ------------- | ------- | ------------- |
| [![Travis](https://travis-ci.org/Project-OSRM/osrm-backend.png?branch=master)](https://travis-ci.org/Project-OSRM/osrm-backend) | [![AppVeyor](https://ci.appveyor.com/api/projects/status/4iuo3s9gxprmcjjh)](https://ci.appveyor.com/project/DennisOSRM/osrm-backend) | [![Codecov](https://codecov.io/gh/Project-OSRM/osrm-backend/branch/master/graph/badge.svg)](https://codecov.io/gh/Project-OSRM/osrm-backend) |

High performance routing engine written in C++14 designed to run on OpenStreetMap data.

The following services are available via HTTP API, C++ library interface and NodeJs wrapper:
- Nearest - Snaps coordinates to the street network and returns the nearest matches
- Route - Finds the fastest route between coordinates
- Table - Computes the duration of the fastest route between all pairs of supplied coordinates
- Match - Snaps noisy GPS traces to the road network in the most plausible way
- Trip - Solves the Traveling Salesman Problem using a greedy heuristic
- Tile - Generates Mapbox Vector Tiles with internal routing metadata

To quickly try OSRM use our [demo server](http://map.project-osrm.org) which comes with both the backend and a frontend on top.

Related [Project-OSRM](https://github.com/Project-OSRM) repositories:
- [node-osrm](https://github.com/Project-OSRM/node-osrm) - Production-ready NodeJs bindings for the routing engine
- [osrm-frontend](https://github.com/Project-OSRM/osrm-frontend) - User-facing frontend with map. The demo server runs this on top of the backend
- [osrm-text-instructions](https://github.com/Project-OSRM/osrm-text-instructions) - Text instructions from OSRM route response

## Contact

- IRC: `irc.oftc.net`, channel: `#osrm` ([Webchat](https://webchat.oftc.net))
- Mailinglist: `https://lists.openstreetmap.org/listinfo/osrm-talk`

## Quick Start

The following targets Ubuntu 16.04.
For instructions how to build on different distributions, macOS or Windows see our [Wiki](https://github.com/Project-OSRM/osrm-backend/wiki).

#### Install dependencies

```bash
sudo apt install build-essential git cmake pkg-config \
libbz2-dev libstxxl-dev libstxxl1v5 libxml2-dev \
libzip-dev libboost-all-dev lua5.2 liblua5.2-dev libtbb-dev
```

#### Compile and install OSRM binaries:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
sudo cmake --build . --target install
```

#### Grab a `.osm.pbf` extract from [Geofabrik](http://download.geofabrik.de/index.html) or [Mapzen's Metro Extracts](https://mapzen.com/data/metro-extracts/)

```bash
wget http://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf
```

#### Pre-process the extract and start the HTTP server

```
osrm-extract berlin-latest.osm.pbf -p profiles/car.lua
osrm-contract berlin-latest.osrm
osrm-routed berlin-latest.osrm
```

#### Running Queries

```
curl http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true
```

## Documentation

### Full documentation

- [Hosted documentation](http://project-osrm.org)
- [osrm-routed HTTP API documentation](docs/http.md)
- [libosrm API documentation](docs/libosrm.md)


### Running a request against the Demo Server

Read the [API usage policy](https://github.com/Project-OSRM/osrm-backend/wiki/Api-usage-policy).
Simple query with instructions and alternatives on Berlin:

```
curl https://router.project-osrm.org/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true&alternatives=true
```

## References in publications

When using the code in a (scientific) publication, please cite

```
@inproceedings{luxen-vetter-2011,
 author = {Luxen, Dennis and Vetter, Christian},
 title = {Real-time routing with OpenStreetMap data},
 booktitle = {Proceedings of the 19th ACM SIGSPATIAL International Conference on Advances in Geographic Information Systems},
 series = {GIS '11},
 year = {2011},
 isbn = {978-1-4503-1031-4},
 location = {Chicago, Illinois},
 pages = {513--516},
 numpages = {4},
 url = {http://doi.acm.org/10.1145/2093973.2094062},
 doi = {10.1145/2093973.2094062},
 acmid = {2094062},
 publisher = {ACM},
 address = {New York, NY, USA},
}
```
