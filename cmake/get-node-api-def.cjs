#!/usr/bin/env node
// Helper used by src/nodejs/CMakeLists.txt to locate the node_api.def file
// that is bundled inside the node-api-headers package (a dependency of cmake-js).
// We resolve it through cmake-js's own require context so the lookup always
// finds the copy that cmake-js expects, regardless of npm hoisting.
'use strict'

const Module = require('module')
const path = require('path')

const cmakeJsPkg = require.resolve(
    path.join(__dirname, '..', 'node_modules', 'cmake-js', 'package.json'))
const req = Module.createRequire(cmakeJsPkg)
const defPath = req('node-api-headers').def_paths.node_api_def

process.stdout.write(defPath)
