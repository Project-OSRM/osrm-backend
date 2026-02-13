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

# Move the built site to build/docs
mv docs/.vitepress/dist build/docs
