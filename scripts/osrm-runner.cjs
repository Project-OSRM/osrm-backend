#!/usr/bin/env node

'use strict';

const fs = require('fs');
const http = require('http');
const process = require('process');
const cla = require('command-line-args');
const clu = require('command-line-usage');
const ansi = require('ansi-escape-sequences');
const turf = require('turf');
const jp = require('jsonpath');
const csv_stringify = require('csv-stringify/lib/sync');

const run_query = (query_options, filters, callback) => {
    let tic = () => 0.;
    http.request(query_options, function (res) {
        let body = '', ttfb = tic();

        res.setEncoding('utf8');
        res.on('data', function (chunk) {
            body += chunk;
        });
        res.on('end', function () {
            const elapsed = tic();
            const json = JSON.parse(body);
            Promise.all(filters.map(filter => jp.query(json, filter)))
                .then(values => callback(query_options.path, res.statusCode, ttfb, elapsed, values));
        });
    }).on('socket', function (/*res*/) {
        tic = ((toc) => { return () => { const t = process.hrtime(toc); return t[0] * 1000 + t[1] / 1000000; }; })(process.hrtime());
    }).on('error', function (res) {
        callback(query_options.path, res.code);
    }).end();
};

function generate_points(polygon, number, coordinates_number, max_distance) {
    let query_points = [];
    while (query_points.length < number) {
        let points = [];

        while(points.length < coordinates_number) {
            let chunk = turf
                .random('points', coordinates_number, { bbox: turf.bbox(polygon)})
                .features
                .map(x => x.geometry.coordinates)
                .filter(pt => turf.inside(pt, polygon));

            if (max_distance > 0)
            {
                chunk.forEach(pt => {
                    if (points.length == 0)
                    {
                        points.push(pt);
                    }
                    else
                    {
                        let distance = turf.distance(pt, points[points.length-1], 'meters');
                        if (distance < max_distance)
                        {
                            points.push(pt);
                        }
                    }
                });
            }
            else
            {
                points = points.concat(chunk);
            }
        }

        query_points.push(points);
    }

    return query_points;
}

function generate_queries(options, query_points) {
    let queries = query_points.map(points => {
        return options.path.replace(/{}/g, x =>  points.pop().join(','));
    });
    return queries;
}

// Command line arguments
function ServerDetails(x) {
    if (!(this instanceof ServerDetails)) return new ServerDetails(x);
    const v = x.split(':');
    this.hostname = (v[0].length > 0) ? v[0] : '';
    this.port = (v.length > 1) ? Number(v[1]) : 80;
}
function BoundingBox(x) {
    if (!(this instanceof BoundingBox)) return new BoundingBox(x);
    const v = x.match(/[+-]?\d+(?:\.\d*)?|\.\d+/g);
    this.poly = turf.bboxPolygon(v.slice(0,4).map(x => Number(x)));
}
const optionsList = [
    {name: 'help', alias: 'h', type: Boolean, description: 'Display this usage guide.', defaultValue: false},
    {name: 'server', alias: 's', type: ServerDetails, defaultValue: ServerDetails('localhost:5000'), description: 'OSRM routing server', typeLabel: '{underline hostname[:port]}'},
    {name: 'path', alias: 'p', type: String, defaultValue: '/route/v1/driving/{};{}', description: 'OSRM query path with \\{\\} coordinate placeholders, default /route/v2/driving/\\{\\};\\{\\}', typeLabel: '{underline path}'},
    {name: 'filter', alias: 'f', type: String, defaultValue: ['$.routes[0].weight'], multiple: true, description: 'JSONPath filters, default "$.routes[0].weight"', typeLabel: '{underline filter}'},
    {name: 'bounding-box', alias: 'b', type: BoundingBox, defaultValue: BoundingBox('5.86442,47.2654,15.0508,55.1478'), multiple: true, description: 'queries bounding box, default "5.86442,47.2654,15.0508,55.1478"', typeLabel: '{underline west,south,east,north}'},
    {name: 'max-sockets', alias: 'm', type: Number, defaultValue: 1, description: 'how many concurrent sockets the agent can have open per origin, default 1', typeLabel: '{underline number}'},
    {name: 'number', alias: 'n', type: Number, defaultValue: 10, description: 'number of query points, default 10', typeLabel: '{underline number}'},
    {name: 'max-distance', alias: 'd', type: Number, defaultValue: -1, description: 'maximal distance between coordinates', typeLabel: '{underline number}'},
    {name: 'queries-files', alias: 'q', type: String, description: 'CSV file with queries in the first row', typeLabel: '{underline file}'},
];
const options = cla(optionsList);
if (options.help) {
    const banner =`\
  ____  _______  __  ______  __  ___  ___  _________
 / __ \\\\/ __/ _ \\\\/  |/  / _ \\\\/ / / / |/ / |/ / __/ _ \\\\
/ /_/ /\\\\ \\\\/ , _/ /|_/ / , _/ /_/ /    /    / _// , _/
\\\\____/___/_/|_/_/  /_/_/|_|\\\\____/_/|_/_/|_/___/_/|_|`;
    const usage = clu([
        { content: ansi.format(banner, 'green'), raw: true },
        { header: 'Run OSRM queries and collect results'/*, content: 'Generates something [italic]{very} important.'*/ },
        { header: 'Options', optionList: optionsList }
    ]);
    console.log(usage);
    process.exit(0);
}

// read or generate random queries
let queries = [];
if (options.hasOwnProperty('queries-files')) {
    queries = fs.readFileSync(options['queries-files'])
        .toString()
        .split('\n')
        .map(r => { const match = /^"([^\"]+)"/.exec(r); return match ? match[1] : null; })
        .filter(q => q);
} else {
    const polygon = options['bounding-box'].map(x => x.poly).reduce((x,y) => turf.union(x, y));
    const coordinates_number = (options.path.match(/{}/g) || []).length;
    const query_points = generate_points(polygon, options.number, coordinates_number, options['max-distance']);
    queries = generate_queries(options, query_points);
}
queries = queries.map(q => { return {hostname: options.server.hostname, port: options.server.port, path: q}; });

// run queries
http.globalAgent.maxSockets = options['max-sockets'];
queries.map(query => {
    run_query(query, options.filter, (query, code, ttfb, total, results) => {
        let data = results ? JSON.stringify(results[0]) : '';
        let record = [[query, code, ttfb, total, data]];
        process.stdout.write(csv_stringify(record));
    });
});
