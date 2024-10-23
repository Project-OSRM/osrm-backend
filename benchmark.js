var OSRM = require('.');
var monaco_mld_path = require('./test/nodejs/constants').mld_data_path;
var two_test_coordinates = require('./test/nodejs/constants').two_test_coordinates;
const { performance } = require('perf_hooks');

const osrm = new OSRM({path: monaco_mld_path, algorithm: 'MLD'});

const numberOfRoutes = 10;
let completedRoutes = 0;
let totalTime = 0;

function benchmarkRoutes() {
    const startTime = performance.now();

    for (let i = 0; i < numberOfRoutes; i++) {

        const options = {
            coordinates: [two_test_coordinates[0], two_test_coordinates[1]],
            annotations: ['distance']
        };

        for (let i = 0; i < 1000; ++i) {
            options.coordinates.push(two_test_coordinates[i % 2], two_test_coordinates[(i + 1) % 2]);
        }

        osrm.table(options, function(err, route) {
            if (err) {
                console.error(err);
                return;
            }
            completedRoutes++;
            if (completedRoutes === numberOfRoutes) {
                const endTime = performance.now();
                totalTime = endTime - startTime;
                console.log(`Total time for ${numberOfRoutes} routes: ${totalTime}ms`);
                console.log(`Average time per route: ${totalTime / numberOfRoutes}ms`);
            }
        });
    }
}

benchmarkRoutes();
