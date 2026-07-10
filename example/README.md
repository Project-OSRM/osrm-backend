## Introduction

This document describes how to build the example programs linked against libosrm.

Every command snippet in this document assumes the starting $PWD as the main project's root level.

#### Test data preparation

Assuming you have Node.js installed:

```bash
cd test/data/
make all
cd ../../
```

### `example.cpp`

After building and installing the main project (on this repo's root level):

```bash
cd example/
mkdir build && cd build/
cmake ..
cmake --build .
./osrm-example ../../test/data/mld/monaco.osrm
```

The expected output is the distance and duration of the route set in the `example.cpp`.

### `example.js`

To run this module in Node.js you need to pass the flag `-DENABLE_NODE_BINDINGS=On` when building the main project.

Refer to the sub-section on preparing test data for the application in this document.

Install the required packages after building the main project and serve the application:

```bash
npm install
cd example/
node example.js
```

You can reliably change the thread pool size of the process by setting the `UV_THREADPOOL_SIZE` variable before calling node:

```bash
UV_THREADPOOL_SIZE=16 node example.js
```

See https://nodejs.org/docs/latest-v20.x/api/os.html#osavailableparallelism for more info.

Then you can test it with:

```bash
curl 'http://localhost:8888?start=7.419758,43.731142&end=7.419505,43.736825'
```

### `trip.html`

`trip.html` is a static browser demo for the Trip service. It talks directly to
an `osrm-routed` HTTP endpoint, uses a vector-tile basemap, and supports click
to add, drag to move, and click/delete to remove markers.

Open it from any static web server and point the `osrm-routed base URL` field
at your running `osrm-routed` instance.
