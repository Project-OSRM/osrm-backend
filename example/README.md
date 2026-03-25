## Introduction

This document describes how to build the example programs linked against libosrm.

Every command in this document assumes the starting $PWD as the main project's root level.

### `example.cpp`

After building and installing the main project (on this repo's root level):

```bash
cd example
cmake ..
cmake --build .
```

#### Test data preparation

If you want to use the included test data and you have Node.js installed:

```bash
cd ../test/data
make all
cd ../../example
```

#### Program execution

```bash
cd example
./osrm-example ../test/data/mld/monaco.osrm
```

The expected output is the distance and duration of the route set in the `example.cpp`.

### `example.js`

To run this module in Node.js you have to pass the flag `-DENABLE_NODE_BINDINGS=On` when building the main project.

Refer to the sub-section on preparing test data for the application in this document.

Install the required packages after building the main project and serve the application:

```bash
npm install
cd example
node example.js
```

Then you can test the application with:

```bash
curl 'http://localhost:8888?start=7.419758,43.731142&end=7.419505,43.736825'
```
