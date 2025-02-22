## Open Source Routing Machine


[![osrm-backend CI](https://github.com/Project-OSRM/osrm-backend/actions/workflows/osrm-backend.yml/badge.svg)](https://github.com/Project-OSRM/osrm-backend/actions/workflows/osrm-backend.yml) [![Codecov](https://codecov.io/gh/Project-OSRM/osrm-backend/branch/master/graph/badge.svg)](https://codecov.io/gh/Project-OSRM/osrm-backend) [![Discord](https://img.shields.io/discord/1034487840219860992)](https://discord.gg/es9CdcCXcb)

High performance routing engine written in C++ designed to run on OpenStreetMap data.

The following services are available via HTTP API, C++ library interface and NodeJs wrapper:
- Nearest - Snaps coordinates to the street network and returns the nearest matches
- Route - Finds the fastest route between coordinates
- Table - Computes the duration or distances of the fastest route between all pairs of supplied coordinates
- Match - Snaps noisy GPS traces to the road network in the most plausible way
- Trip - Solves the Traveling Salesman Problem using a greedy heuristic
- Tile - Generates Mapbox Vector Tiles with internal routing metadata

To quickly try OSRM use our [demo server](http://map.project-osrm.org) which comes with both the backend and a frontend on top.

For a quick introduction about how the road network is represented in OpenStreetMap and how to map specific road network features have a look at [the OSM wiki on routing](https://wiki.openstreetmap.org/wiki/Routing) or [this guide about mapping for navigation](https://web.archive.org/web/20221206013651/https://labs.mapbox.com/mapping/mapping-for-navigation/).

Related [Project-OSRM](https://github.com/Project-OSRM) repositories:
- [osrm-frontend](https://github.com/Project-OSRM/osrm-frontend) - User-facing frontend with map. The demo server runs this on top of the backend
- [osrm-text-instructions](https://github.com/Project-OSRM/osrm-text-instructions) - Text instructions from OSRM route response
- [osrm-backend-docker](https://github.com/project-osrm/osrm-backend/pkgs/container/osrm-backend) - Ready to use Docker images

## Documentation

### Full documentation

- [Hosted documentation](http://project-osrm.org)
- [osrm-routed HTTP API documentation](docs/http.md)
- [libosrm API documentation](docs/libosrm.md)

## Contact

- Discord: [join](https://discord.gg/es9CdcCXcb)
- IRC: `irc.oftc.net`, channel: `#osrm` ([Webchat](https://webchat.oftc.net))
- Mailinglist: `https://lists.openstreetmap.org/listinfo/osrm-talk`

## Quick Start

The easiest and quickest way to setup your own routing engine is to use Docker images we provide.

There are two pre-processing pipelines available:
- Contraction Hierarchies (CH)
- Multi-Level Dijkstra (MLD)

we recommend using MLD by default except for special use-cases such as very large distance matrices where CH is still a better fit for the time being.
In the following we explain the MLD pipeline.
If you want to use the CH pipeline instead replace `osrm-partition` and `osrm-customize` with a single `osrm-contract` and change the algorithm option for `osrm-routed` to `--algorithm ch`.

### Using Docker

We base our Docker images ([backend](https://github.com/Project-OSRM/osrm-backend/pkgs/container/osrm-backend), [frontend](https://hub.docker.com/r/osrm/osrm-frontend/)) on Debian and make sure they are as lightweight as possible. Older backend versions can be found on [Docker Hub](https://hub.docker.com/r/osrm/osrm-backend/).

Download OpenStreetMap extracts for example from [Geofabrik](http://download.geofabrik.de/)

    wget http://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf

Pre-process the extract with the car profile and start a routing engine HTTP server on port 5000

    docker run -t -v "${PWD}:/data" ghcr.io/project-osrm/osrm-backend osrm-extract -p /opt/car.lua /data/berlin-latest.osm.pbf || echo "osrm-extract failed"

The flag `-v "${PWD}:/data"` creates the directory `/data` inside the docker container and makes the current working directory `"${PWD}"` available there. The file `/data/berlin-latest.osm.pbf` inside the container is referring to `"${PWD}/berlin-latest.osm.pbf"` on the host.

    docker run -t -v "${PWD}:/data" ghcr.io/project-osrm/osrm-backend osrm-partition /data/berlin-latest.osrm || echo "osrm-partition failed"
    docker run -t -v "${PWD}:/data" ghcr.io/project-osrm/osrm-backend osrm-customize /data/berlin-latest.osrm || echo "osrm-customize failed"

Note there is no `berlin-latest.osrm` file, but multiple `berlin-latest.osrm.*` files, i.e. `berlin-latest.osrm` is not file path, but "base" path referring to set of files and there is an option to omit this `.osrm` suffix completely(e.g. `osrm-partition /data/berlin-latest`).

    docker run -t -i -p 5000:5000 -v "${PWD}:/data" ghcr.io/project-osrm/osrm-backend osrm-routed --algorithm mld /data/berlin-latest.osrm

Make requests against the HTTP server

    curl "http://127.0.0.1:5000/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true"

Optionally start a user-friendly frontend on port 9966, and open it up in your browser

    docker run -p 9966:9966 osrm/osrm-frontend
    xdg-open 'http://127.0.0.1:9966'

In case Docker complains about not being able to connect to the Docker daemon make sure you are in the `docker` group.

    sudo usermod -aG docker $USER

After adding yourself to the `docker` group make sure to log out and back in again with your terminal.

We support the following images in the Container Registry:

Name | Description
-----|------
`latest` | `master` compiled with release flag
`latest-assertions` | `master` compiled with with release flag, assertions enabled and debug symbols
`latest-debug` | `master` compiled with debug flag
`<tag>` | specific tag compiled with release flag
`<tag>-debug` | specific tag compiled with debug flag

### Building from Source

The following targets Ubuntu 22.04.
For instructions how to build on different distributions, macOS or Windows see our [Wiki](https://github.com/Project-OSRM/osrm-backend/wiki).

Install dependencies

```bash
sudo apt install build-essential git cmake pkg-config \
libbz2-dev libxml2-dev libzip-dev libboost-all-dev \
lua5.2 liblua5.2-dev libtbb-dev
```

Compile and install OSRM binaries

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
sudo cmake --build . --target install
```

### Request Against the Demo Server

Read the [API usage policy](https://github.com/Project-OSRM/osrm-backend/wiki/Demo-server).

Simple query with instructions and alternatives on Berlin:

```
curl "https://router.project-osrm.org/route/v1/driving/13.388860,52.517037;13.385983,52.496891?steps=true&alternatives=true"
```

### Using the Node.js Bindings

The Node.js bindings provide read-only access to the routing engine.
We provide API documentation and examples [here](docs/nodejs/api.md).

You will need a modern `libstdc++` toolchain (`>= GLIBCXX_3.4.26`) for binary compatibility if you want to use the pre-built binaries.
For older Ubuntu systems you can upgrade your standard library for example with:

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update -y
sudo apt-get install -y libstdc++-9-dev
```

You can install the Node.js bindings via `npm install @project-osrm/osrm` or from this repository either via

    npm install

which will check and use pre-built binaries if they're available for this release and your Node version, or via

    npm install --build-from-source

to always force building the Node.js bindings from source.

#### Unscoped packages

Prior to v5.27.0, the `osrm` Node package was unscoped. If you are upgrading from an old package, you will need to do the following:

```
npm uninstall osrm --save
npm install @project-osrm/osrm --save
```

#### Package docs

For usage details have a look [these API docs](docs/nodejs/api.md).

An exemplary implementation by a 3rd party with Docker and Node.js can be found [here](https://github.com/door2door-io/osrm-express-server-demo).


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
