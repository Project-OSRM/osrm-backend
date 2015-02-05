# OSM Testdata

This directory contains software that can be used with the osm-testdata
repository at https://github.com/osmcode/osm-testdata . To use it, clone
the `osm-testdata` repository in the same directory where you cloned the
`libosmium` repository.

## Overview

The `testdata-overview` program can be used to create a Spatialite file
containing all the nodes and ways from the test data files.

Compile it by running `make testdata-overview`, run it my calling
`make overview`.

## Running the Tests

Actual tests are in `testcases` subdirectory, one per test from the
osm-testdata repository.

To compile the tests, call `make runtests`, to run them call
`make test`.

