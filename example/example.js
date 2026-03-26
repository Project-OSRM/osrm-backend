import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

import express from 'express';

import OSRM from '../lib/index.js';

const app = express();
const __dirname = dirname(fileURLToPath(import.meta.url));
const osrm = new OSRM(join(__dirname,"../test/data/ch/monaco.osrm"));

// Accepts a query like:
// http://localhost:8888?start=7.419758,43.731142&end=7.419505,43.736825
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

const port = 8888;
console.log('Listening on port: ' + port);
app.listen(port);
