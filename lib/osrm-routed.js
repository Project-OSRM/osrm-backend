#!/usr/bin/env node
process.env.UV_THREADPOOL_SIZE = Math.ceil(require('os').cpus().length * 1.5);

var express = require('express');
var OSRM = require('..');
var path = require('path');

var app = express();
var osrm;

var argv = require('minimist')(process.argv.slice(2));

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
-p [ --port ] arg (=5000)             TCP/IP port
-t [ --threads ] arg (=4)             Number of threads to use
-s [ --shared-memory ] [=arg(=1)] (=0)
                                      Load data from shared memory
-a [ --algorithm ] arg (=CH)          Algorithm to use for the data. Can be
                                      CH, CoreCH, MLD.
--max-viaroute-size arg (=500)        Max. locations supported in viaroute
                                      query
--max-trip-size arg (=100)            Max. locations supported in trip query
--max-table-size arg (=100)           Max. locations supported in distance
                                      table query
--max-matching-size arg (=100)        Max. locations supported in map
                                      matching query
--max-nearest-size arg (=100)         Max. results supported in nearest query
--max-alternatives arg (=3)           Max. number of alternatives supported
                                      in the MLD route query
`);

    return process.exit(0);
}
console.dir(argv);

//--shared-memory=1 -p 5000 -a CH

var algorithm = 'CH';
if ('a' in argv) { algorithm = argv.a; }
if ('algorithm' in argv) { algorithm = argv.algorithm; }

var port = 5000;
if ('p' in argv) { port = parseInt(argv.p); }
if ('port' in argv) { port = parseInt(argv.port); }

if (argv._.length == 1) {
    console.log("Loading "+argv._[0]);
    osrm = new OSRM({path: argv._[0], algorithm: algorithm});
} else {
    console.log("Using shared memory");
    osrm = new OSRM({algorithm: algorithm}); // using datastore
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

console.log('Listening on port: ' + port);
app.listen(port);
