const OSRM = require('../../');
const {performance, createHistogram} = require('node:perf_hooks');

// usage: node test/nodejs/benchmark.js berlin-latest.osrm 13.388860,52.517037;13.385983,52.496891
const args = process.argv.slice(2);
const path = args[0] || require('./constants').mld_data_path;

function parseWaypoints(waypoints) {
    if (waypoints == undefined) {
        return undefined;
    }
    return waypoints.split(';').map((waypoint) => {
        const [lon, lat] = waypoint.split(',');
        return [parseFloat(lon), parseFloat(lat)];
    });
}
const waypoints = parseWaypoints(args[1]) || [[7.41337, 43.72956],[7.41546, 43.73077]];
const osrm = new OSRM({path, algorithm: 'MLD'});

async function route(coordinates) {
    const promise = new Promise((resolve, reject) => {
        osrm.route({coordinates, steps: true, overview: 'full'}, (err, result) => {
            if (err) {
                reject(err);
            } else {
                resolve(result);
            }
        });
    });
    return promise;
}

async function benchmark() {
    // warmup
    await route(waypoints);

    const performanceHistorgram = createHistogram();

    for (let i = 0; i < 1000; i++) {
        const start = performance.now();
        await route(waypoints);
        const end = performance.now();
        // record result in microseconds
        performanceHistorgram.record(Math.ceil((end - start) * 1000));
    }


    console.log(performanceHistorgram);
}
benchmark();

