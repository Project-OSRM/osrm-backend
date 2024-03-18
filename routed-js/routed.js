#!/usr/bin/env node
"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const fastify_1 = __importDefault(require("fastify"));
const OSRMWrapper_1 = require("./OSRMWrapper");
const yargs_1 = __importDefault(require("yargs/yargs"));
const cluster_1 = __importDefault(require("cluster"));
const os_1 = __importDefault(require("os"));
const schema_1 = require("./schema");
const MatchServiceHandler_1 = require("./MatchServiceHandler");
const NearestServiceHandler_1 = require("./NearestServiceHandler");
const RouteServiceHandler_1 = require("./RouteServiceHandler");
const TableServiceHandler_1 = require("./TableServiceHandler");
const TripServiceHandler_1 = require("./TripServiceHandler");
const Format_1 = require("./Format");
async function main() {
    const argv = await (0, yargs_1.default)(process.argv.slice(2)).options({
        ip: { type: 'string', default: '0.0.0.0', alias: 'i' },
        port: { type: 'number', default: 5000, alias: 'p' },
        workers: { type: 'number', alias: ['t', 'threads'], default: os_1.default.cpus().length },
        shared_memory: { type: 'boolean', alias: ['shared-memory', 's'] },
        mmap: { type: 'boolean', default: false, alias: ['m'] },
        algorithm: { choices: ['CH', 'CoreCH', 'MLD'], default: 'CH', alias: 'a' },
        dataset_name: { type: 'string', alias: 'dataset-name' },
        max_viaroute_size: { type: 'number', alias: 'max-viaroute-size', default: 500 },
        max_trip_size: { type: 'number', alias: 'max-trip-size', default: 100 },
        max_table_size: { type: 'number', alias: 'max-table-size', default: 100 },
        max_matching_size: { type: 'number', alias: 'max-matching-size', default: 100 },
        max_nearest_size: { type: 'number', alias: 'max-nearest-size', default: 100 },
        max_alternatives: { type: 'number', alias: 'max-alternatives', default: 3 },
        max_matching_radius: { type: 'number', alias: 'max-matching-radius', default: -1 },
        version: { alias: 'v' }
    })
        .help('h')
        .alias('h', 'help')
        .strict()
        .argv;
    if (argv.version) {
        process.stdout.write(`v${OSRMWrapper_1.version}\n`);
        return;
    }
    if (argv._.length == 0 && !argv.shared_memory) {
        // TODO: show usage
        return;
    }
    const osrm = new OSRMWrapper_1.OSRMWrapper({
        path: argv._[0],
        dataset_name: argv.dataset_name,
        algorithm: argv.algorithm,
        shared_memory: argv.shared_memory,
        mmap_memory: argv.mmap,
        max_viaroute_size: argv.max_viaroute_size,
        max_trip_size: argv.max_trip_size,
        max_table_size: argv.max_table_size,
        max_matching_size: argv.max_matching_size,
        max_nearest_size: argv.max_nearest_size,
        max_alternatives: argv.max_alternatives,
        max_matching_radius: argv.max_matching_size
    });
    const fastify = (0, fastify_1.default)({
        logger: true,
        maxParamLength: Number.MAX_SAFE_INTEGER,
        rewriteUrl: (req) => {
            // https://github.com/fastify/fastify/issues/2487
            return req.url.replace(/;/g, '%3B');
        },
        querystringParser: schema_1.parseQueryString
    });
    await fastify.register(Promise.resolve().then(() => __importStar(require('@fastify/compress'))));
    async function processRequest(handler, request, reply) {
        const { coordinatesAndFormat } = request.params;
        const query = request.query;
        try {
            const { format, coordinates } = (0, schema_1.parseCoordinatesAndFormat)(coordinatesAndFormat);
            switch (format) {
                case Format_1.Format.Json:
                    reply.type('application/json').code(200);
                    break;
                case Format_1.Format.Flatbuffers:
                    reply.type('application/x-flatbuffers;schema=osrm.engine.api.fbresult').code(200);
                    break;
            }
            return handler.handle(coordinates, query, format);
        }
        catch (e) {
            reply.code(400);
            // TODO: bindings do not return `message`, but put `code` into `message`
            return {
                code: e.message,
                message: e.message
            };
        }
    }
    fastify.get('/route/v1/:profile/:coordinatesAndFormat', { schema: schema_1.routeSchema }, async (request, reply) => {
        return processRequest(new RouteServiceHandler_1.RouteServiceHandler(osrm), request, reply);
    });
    fastify.get('/nearest/v1/:profile/:coordinatesAndFormat', { schema: schema_1.nearestSchema }, async (request, reply) => {
        return processRequest(new NearestServiceHandler_1.NearestServiceHandler(osrm), request, reply);
    });
    fastify.get('/table/v1/:profile/:coordinatesAndFormat', { schema: schema_1.tableSchema }, async (request, reply) => {
        return processRequest(new TableServiceHandler_1.TableServiceHandler(osrm), request, reply);
    });
    fastify.get('/match/v1/:profile/:coordinatesAndFormat', { schema: schema_1.matchSchema }, async (request, reply) => {
        return processRequest(new MatchServiceHandler_1.MatchServiceHandler(osrm), request, reply);
    });
    fastify.get('/trip/v1/:profile/:coordinatesAndFormat', { schema: schema_1.tripSchema }, async (request, reply) => {
        return processRequest(new TripServiceHandler_1.TripServiceHandler(osrm), request, reply);
    });
    fastify.get('/tile/v1/:profile/tile(:x,:y,:zoom).mvt', { schema: schema_1.tileSchema }, async (request, reply) => {
        const { x, y, zoom } = request.params;
        reply.type('application/x-protobuf').code(200);
        return osrm.tile([zoom, x, y]);
    });
    const start = async () => {
        try {
            await fastify.listen({ port: argv.port, host: argv.ip });
            process.stdout.write('running and waiting for requests\n');
        }
        catch (err) {
            fastify.log.error(err);
            process.exit(1);
        }
    };
    const clusterWorkerSize = argv.workers;
    if (clusterWorkerSize > 1) {
        if (cluster_1.default.isMaster) {
            for (let i = 0; i < clusterWorkerSize; i++) {
                cluster_1.default.fork();
            }
            cluster_1.default.on("exit", function (worker) {
                console.log("Worker", worker.id, " has exited.");
            });
        }
        else {
            start();
        }
    }
    else {
        start();
    }
}
main();
