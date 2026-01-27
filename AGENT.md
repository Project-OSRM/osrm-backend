## Open Source Routing Machine

OSRM (Open Source Routing Machine) is a high performance routing engine written in C++ designed to run on OpenStreetMap data.
It provides various routing services including route finding, distance/duration tables, GPS trace matching, and traveling salesman problem solving.
It is accessible via HTTP API, C++ library interface, and Node.js wrapper.
OSRM supports two routing algorithms: Contraction Hierarchies (CH) and Multi-Level Dijkstra (MLD).

## Developing

We target C++20 but need to deal with older compilers that only have partial support.

Use `./scripts/format.sh` to format the code. Ensure clang-format-15 is available on the system.

## Building

This project uses CMake. The build directory should be `build` or `build-{something}` or `build/{something}/`. For example:
- `./build`
- `./build-gcc-debug`
- `./build/gcc-debug`
- `./build/gcc`
- `./build/clang-debug`
- `./build-clang-release` 

Use `cmake --build ${BUILD_DIR} --target ${TARGET} -j $(nproc --ignore=2)` to build a target.


## Running

OSRM supports two data preparations: Contraction Hierachies (CH) and Mulit-Level-Dijkstra (MLD).

For CH:
1. `./${BUILD_DIR}/osrm-extract -p profiles/car.lua data.osm.pbf`
2. `./${BUILD_DIR}/osrm-partition data.osrm` (optional, improves CH performance due to cache locallity of node reordering)
3. `./${BUILD_DIR}/osrm-contract data.osrm`
4. `./${BUILD_DIR}/osrm-routed -a CH data.osrm`

For MLD:
1. `./${BUILD_DIR}/osrm-extract -p profiles/car.lua data.osm.pbf`
2. `./${BUILD_DIR}/osrm-partition data.osrm` (optional, improves CH performance due to cache locallity of node reordering)
3. `./${BUILD_DIR}/osrm-customize data.osrm`
4. `./${BUILD_DIR}/osrm-routed -a MLD data.osrm`

A dataset can be both MLD and CH if both `osrm-contract` and `osrm-customize` are executed:
1. `./${BUILD_DIR}/osrm-extract -p profiles/car.lua data.osm.pbf`
2. `./${BUILD_DIR}/osrm-partition data.osrm`
3. `./${BUILD_DIR}/osrm-contract data.osrm`
4. `./${BUILD_DIR}/osrm-customize data.osrm`
5. `./${BUILD_DIR}/osrm-routed -a CH data.osrm` OR `./${BUILD_DIR}/osrm-routed -a MLD data.osrm`


## Testing

### Running Unit tests

Unit tests are contained in `unit_tests/` using boost test and can be build with:
```bash
cmake --build ${BUILD_DIR} --target tests
```

This will create various tests in `${BUILD_DIR}/unit_tests/`.

### Running Integration Tests

Integration tests are contained in `features/` using cucumber-js

To run cucumber tests with the full test matrix across different algorithms (CH, MLD) and load methods (mmap, directly, datastore).
```bash
npm test
```

To run cucumber tests for a specific configuration:
```bash
npx cucumber-js -p home -p ch -p datastore
npx cucumber-js -p home -p mld -p mmap
```


## Contributing

<IMPORTANT>
Use the [PR template](.github/PULL_REQUEST_TEMPLATE.md) to build the PR description.
Ensure the description incudes which AI tool and model was used, add a robot emoji to the description.
</IMPORTANT>

