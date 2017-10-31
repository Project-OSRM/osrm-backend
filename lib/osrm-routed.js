#!/usr/bin/env node
process.env.UV_THREADPOOL_SIZE = Math.ceil(require('os').cpus().length * 1.5);

var express = require('express');
var OSRM = require('..');
var path = require('path');

var app = express();
var osrm;

var argv = require('minimist')(process.argv.slice(2));

// Defaults
var config = {
    algorithm: 'CH',
    max_locations_viaroute: 500,
    max_locations_trip: 100,
    max_locations_distance_table: 100,
    max_locations_map_matching: 100,
    max_results_nearest: 100,
    max_alternatives: 3
};
var port = 5000;

if (Object.keys(argv).length == 1 && argv._.length == 0 || ('help' in argv && argv.help)) {
console.log(`
Options:
-v [ --version ]                      Show version
-h [ --help ]                         Show this help message
-l [ --verbosity ] arg (=INFO)        Log verbosity level: NONE, ERROR,
                                      WARNING, INFO, DEBUG
--trial [=arg(=1)]                    Quit after initialization

Configuration:
-i [ --ip ] arg (=0.0.0.0)            IP address
-p [ --port ] arg (=${port})             TCP/IP port
-t [ --threads ] arg (=${process.env.UV_THREADPOOL_SIZE})             Number of threads to use
-s [ --shared-memory ] [=arg(=1)] (=0)
                                      Load data from shared memory
-a [ --algorithm ] arg (=${config.algorithm})          Algorithm to use for the data. Can be
                                      CH, CoreCH, MLD
--max-viaroute-size arg (=${config.max_locations_viaroute})        Max. locations supported in viaroute
                                      query
--max-trip-size arg (=${config.max_locations_trip})            Max. locations supported in trip query
--max-table-size arg (=${config.max_locations_distance_table})           Max. locations supported in distance
                                      table query
--max-matching-size arg (=${config.max_locations_map_matching})        Max. locations supported in map
                                      matching query
--max-nearest-size arg (=${config.max_results_nearest})         Max. results supported in nearest query
--max-alternatives arg (=${config.max_alternatives})           Max. number of alternatives supported
                                      in the MLD route query
`);

    return process.exit(0);
}

if ('v' in argv || 'version' in argv) {
    console.log(`v${OSRM.version}`);
    return process.exit(0);
}

if ('a' in argv) { config.algorithm = argv.a; }
if ('algorithm' in argv) { config.algorithm = argv.algorithm; }

if ('p' in argv) { port = parseInt(argv.p); }
if ('port' in argv) { port = parseInt(argv.port); }

var hostname = null;
if ('i' in argv) { hostname = argv.i; }
if ('ip' in argv) { hostname = argv.ip; }

if ('t' in argv) { process.env.UV_THREADPOOL_SIZE = parseInt(argv.t); }
if ('threads' in argv) { process.env.UV_THREADPOOL_SIZE = parseInt(argv.threads); }

if ('max-viaroute-size' in argv) { config.max_locations_viaroute = parseInt(argv['max-viaroute-size']); }
if ('max-trip-size' in argv) { config.max_locations_trip = parseInt(argv['max-trip-size']); }
if ('max-table-size' in argv) { config.max_locations_distance_table = parseInt(argv['max-table-size']); }
if ('max-matching-size' in argv) { config.max_locations_map_matching = parseInt(argv['max-matching-size']); }
if ('max-nearest-size' in argv) { config.max_results_nearest = parseInt(argv['max-nearest-size']); }
if ('max-alternatives' in argv) { config.max_alternatives = parseInt(argv['max-alternatives']); }

console.log(`[info] starting up engines, v${OSRM.version}`);
console.log(`[info] Threads: ${process.env.UV_THREADPOOL_SIZE}`);
console.log(`[info] IP address: ${hostname || '0.0.0.0'}`);
console.log(`[info] IP port: ${port}`);

if ('s' in argv || 'shared-memory' in argv) {
    if (argv._.length > 0) {
        console.error('Error: can\'t use shared memory and supply a filename');
        return process.exit(1);
    }
    console.log('[info] Loading from shared memory')
    osrm = new OSRM(config); // using datastore
} else {
    config.path = argv._[0];
    osrm = new OSRM(config);
}

// Accepts a query like:
// http://localhost:8888/route/v1/driving/1,1;2,2
var middleware = OSRM.middleware(osrm);
app.get('/route/v1/:profile/:coordinates', (req,res,next) => { console.log(req.url); next(); }, middleware.route);
app.get('/nearest/v1/:profile/:coordinates', (req,res,next) => { console.log(req.url); next(); }, middleware.nearest);
app.get('/table/v1/:profile/:coordinates', (req,res,next) => { console.log(req.url); next(); }, middleware.table);
app.get('/match/v1/:profile/:coordinates', (req,res,next) => { console.log(req.url); next(); }, middleware.match);
app.get('/trip/v1/:profile/:coordinates', (req,res,next) => { console.log(req.url); next(); }, middleware.trip);

app.use((req, res, next) => { res.status(404).json({'code':'NotFound','message':'Path not found'}); });

app.listen(port,hostname, (err) => {
    if (err) {
        console.error(err);
        process.exit(1);
    }
    if (process.env.SIGNAL_PARENT_WHEN_READY) {
        console.warn('Support for SIGNAL_PARENT_WHEN_READY not yet implemented - no signal sent');
        // Needs https://github.com/nodejs/node/issues/14957
    }
    console.log(`[info] Listening on: ${hostname || '0.0.0.0'}:${port}`);
    console.log('[info] running and waiting for requests');
});
