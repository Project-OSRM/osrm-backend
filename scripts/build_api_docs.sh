#!/bin/bash

set -e -u

if [ ! -f docs/http.md ] ; then
    echo "This script should be run from the repository root directory"
    exit 1
fi

# Clean up previous version
rm -rf build/docs

# Extract JSDoc comments from C++ and generate API docs
# (replaces --polyglot flag removed in documentation.js v14)
mkdir -p build/docs
node scripts/extract_cpp_jsdoc.js src/nodejs/node_osrm.cpp > build/docs/jsdoc-extract.js
npx documentation build build/docs/jsdoc-extract.js --markdown-toc=false -f md -o docs/nodejs/api.md

# Build static site with VitePress
npx vitepress build docs

# Copy the built site contents to build/docs (site root directly under build/docs)
cp -a docs/.vitepress/dist/. build/docs/
