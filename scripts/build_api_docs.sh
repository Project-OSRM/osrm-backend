#!/bin/bash

set -e -u

if [ ! -f docs/http.md ] ; then
    echo "This script should be run from the repository root directory"
    exit 1
fi

# Check that the required tools are in the PATH somewhere.
# If executed via `npm run build-api-docs`, then node_modules/.bin is in the PATH
# and these tools should be found
babel -V >/dev/null 2>&1 || { echo >&2 "Can't find babel.  Add node_modules/.bin to your path, or run via \"npm run\""; exit 1; }
browserify --help >/dev/null 2>&1 || { echo >&2 "Can't find browserify.  Add node_modules/.bin to your path, or run via \"npm run\""; exit 1; }
uglifyjs -V >/dev/null 2>&1 || { echo >&2 "Can't find uglifyjs.  Add node_modules/.bin to your path, or run via \"npm run\""; exit 1; }

documentation build src/nodejs/node_osrm.cpp --polyglot --markdown-toc=false -f md -o docs/nodejs/api.md

# Clean up previous version
rm -rf build/docs

# Make temp dir to hold docbox template
mkdir -p build/docs/tmp/src 

# Copy docbox template scripts into temp dir
cp -r node_modules/docbox/src/* build/docs/tmp/src 
cp -r node_modules/docbox/css build/docs/ 

# Copy our images/templates into the temp docs dir
cp -r docs/images build/docs 
cp docs/src/index.html build/docs/tmp 
cp docs/src/* build/docs/tmp/src/custom 
mkdir -p build/docs/tmp/content 
cp docs/*.md build/docs/tmp/content 

# Now, run the scripts to generate the actual final product
pushd build/docs/tmp 
NODE_ENV=production browserify src/index.js | uglifyjs -c -m > ../bundle.js 
babel src --out-dir lib 
node lib/render.js ../index.html 
popd

# Cleanup
rm -rf build/docs/tmp
