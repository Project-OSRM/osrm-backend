const fs = require('fs');
const path = require('path');
const readline = require('readline');
const seedrandom = require('seedrandom');


let RNG;

class GPSData {
    constructor(gpsTracesFilePath) {
        this.tracks = {};
        this.coordinates = [];
        this.trackIds = [];
        this._loadGPSTraces(gpsTracesFilePath);
    }

    _loadGPSTraces(gpsTracesFilePath) {
        const expandedPath = path.resolve(gpsTracesFilePath);
        const data = fs.readFileSync(expandedPath, 'utf-8');
        const lines = data.split('\n');
        const headers = lines[0].split(',');

        const latitudeIndex = headers.indexOf('Latitude');
        const longitudeIndex = headers.indexOf('Longitude');
        const trackIdIndex = headers.indexOf('TrackID');

        for (let i = 1; i < lines.length; i++) {
            if (lines[i].trim() === '') continue;
            const row = lines[i].split(',');

            const latitude = parseFloat(row[latitudeIndex]);
            const longitude = parseFloat(row[longitudeIndex]);
            const trackId = row[trackIdIndex];

            const coord = [longitude, latitude];
            this.coordinates.push(coord);

            if (!this.tracks[trackId]) {
                this.tracks[trackId] = [];
            }
            this.tracks[trackId].push(coord);
        }

        this.trackIds = Object.keys(this.tracks);
    }

    getRandomCoordinate() {
        const randomIndex = Math.floor(RNG() * this.coordinates.length);
        return this.coordinates[randomIndex];
    }

    getRandomTrack() {
        const randomIndex = Math.floor(RNG() * this.trackIds.length);
        const trackId = this.trackIds[randomIndex];
        return this.tracks[trackId];
    }
};

async function runOSRMMethod(osrm, method, coordinates) {
    const time = await new Promise((resolve, reject) => {
        const startTime = process.hrtime();
        osrm[method]({coordinates}, (err, result) => {
            if (err) {
                if (['NoSegment', 'NoMatch', 'NoRoute', 'NoTrips'].includes(err.message)) {
                    resolve(null);
                } else {

                reject(err);
                }
            } else {
                const endTime = process.hrtime(startTime);
                resolve(endTime[0] + endTime[1] / 1e9);
            }
        });
    });
    return time;
}

async function nearest(osrm, gpsData) {
    const times = [];
    for (let i = 0; i < 1000; i++) {
        const coord = gpsData.getRandomCoordinate();
        times.push(await runOSRMMethod(osrm, 'nearest', [coord]));
    }
    return times;
}

async function route(osrm, gpsData) {
    const times = [];
    for (let i = 0; i < 1000; i++) {
        const from = gpsData.getRandomCoordinate();
        const to = gpsData.getRandomCoordinate();

        
        times.push(await runOSRMMethod(osrm, 'route', [from, to]));
    }
    return times;
}

async function table(osrm, gpsData) {
    const times = [];
    for (let i = 0; i < 250; i++) {
        const numPoints = Math.floor(RNG() * 3) + 15;
        const coordinates = [];
        for (let i = 0; i < numPoints; i++) {
            coordinates.push(gpsData.getRandomCoordinate());
        }

        
        times.push(await runOSRMMethod(osrm, 'table', coordinates));
    }
    return times;
}

async function match(osrm, gpsData) {
    const times = [];
    for (let i = 0; i < 1000; i++) {
        const numPoints = Math.floor(RNG() * 50) + 50;
        const coordinates = gpsData.getRandomTrack().slice(0, numPoints);

        
        times.push(await runOSRMMethod(osrm, 'match', coordinates));
    }
    return times;
}

async function trip(osrm, gpsData) {
    const times = [];
    for (let i = 0; i < 250; i++) {
        const numPoints = Math.floor(RNG() * 2) + 5;
        const coordinates = [];
        for (let i = 0; i < numPoints; i++) {
            coordinates.push(gpsData.getRandomCoordinate());
        }

        
        times.push(await runOSRMMethod(osrm, 'trip', coordinates));
    }
    return times;
}

function bootstrapConfidenceInterval(data, numSamples = 1000, confidenceLevel = 0.95) {
    let means = [];
    let dataLength = data.length;

    for (let i = 0; i < numSamples; i++) {
        let sample = [];
        for (let j = 0; j < dataLength; j++) {
            let randomIndex = Math.floor(RNG() * dataLength);
            sample.push(data[randomIndex]);
        }
        let sampleMean = sample.reduce((a, b) => a + b, 0) / sample.length;
        means.push(sampleMean);
    }

    means.sort((a, b) => a - b);
    let lowerBoundIndex = Math.floor((1 - confidenceLevel) / 2 * numSamples);
    let upperBoundIndex = Math.floor((1 + confidenceLevel) / 2 * numSamples);
    let mean = means.reduce((a, b) => a + b, 0) / means.length;
    let lowerBound = means[lowerBoundIndex];
    let upperBound = means[upperBoundIndex];

    return { mean: mean, lowerBound: lowerBound, upperBound: upperBound };
}

function calculateConfidenceInterval(data) {
    let { mean, lowerBound, upperBound } = bootstrapConfidenceInterval(data);
    let bestValue = Math.max(...data);
    let errorMargin = (upperBound - lowerBound) / 2;

    return { mean, errorMargin, bestValue };
}

async function main() {
    const args = process.argv.slice(2);

    const {OSRM} = require(args[0]);
    const path = args[1];
    const algorithm = args[2].toUpperCase();
    const method = args[3];
    const gpsTracesFilePath = args[4];
    const iterations = parseInt(args[5]);

    const gpsData = new GPSData(gpsTracesFilePath);
    const osrm = new OSRM({path, algorithm});


    const functions = {
        route: route,
        table: table,
        nearest: nearest,
        match: match,
        trip: trip
    };
    const func = functions[method];
    if (!func) {
        throw new Error('Unknown method');
    }
    const allTimes = [];
    for (let i = 0; i < iterations; i++) {
        RNG = seedrandom(42);
        allTimes.push((await func(osrm, gpsData)).filter(t => t !== null));
    }

    const opsPerSec = allTimes.map(times => times.length / times.reduce((a, b) => a + b, 0));
    const { mean, errorMargin, bestValue } = calculateConfidenceInterval(opsPerSec);
    console.log(`Ops: ${mean.toFixed(1)} ± ${errorMargin.toFixed(1)} ops/s. Best: ${bestValue.toFixed(1)} ops/s`);

}

main();
