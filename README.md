
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
- Match - Snaps noisy GPS traces to the road network in the most plausible way
- Trip - Solves the Traveling Salesman Problem using a greedy heuristic
- Tile - Generates Mapbox Vector Tiles with internal routing metadata


To quickly try OSRM use our [demo server](http://map.project-osrm.org) which comes with both the backend and a frontend on top.


For a quick introduction about how the road network is represented in OpenStreetMap and how to map specific road network features have a look at [the OSM wiki on routing](https://wiki.openstreetmap.org/wiki/Routing) or [this guide about mapping for navigation](https://web.archive.org/web/20221206013651/https://labs.mapbox.com/mapping/mapping-for-navigation/).


Related [Project-OSRM](https://github.com/Project-OSRM) repositories:
