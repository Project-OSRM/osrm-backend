process.env.UV_THREADPOOL_SIZE = Math.ceil(require('os').cpus().length * 1.5);

const express = require('express');
const OSRM = require('..');
const path = require('path');

const app = express();
const osrm = new OSRM(path.join(__dirname,"../test/data/ch/monaco.osrm"));

// Accepts a query like:
// http://localhost:8888?start=13.414307,52.521835&end=13.402290,52.523728
app.get('/', function(req, res) {
    if (!req.query.start || !req.query.end) {
        return res.json({"error":"invalid start and end query"});
    }
    const coordinates = [];
    const start = req.query.start.split(',');
    coordinates.push([+start[0],+start[1]]);
    const end = req.query.end.split(',');
    coordinates.push([+end[0],+end[1]]);
    const query = {
        coordinates: coordinates,
        alternateRoute: req.query.alternatives !== 'false'
    };
    osrm.route(query, function(err, result) {
        if (err) return res.json({"error":err.message});
        return res.json(result);
    });
});

console.log('Listening on port: ' + 8888);
app.listen(8888);
