#!/usr/bin/env node
import Fastify, { FastifyReply, FastifyRequest } from 'fastify';
import { OSRMWrapper, version as OSRMVersion } from './OSRMWrapper';
import yargs from 'yargs/yargs';
import cluster from 'cluster';
import os from 'os';
import { routeSchema, nearestSchema, tableSchema, tripSchema, matchSchema, tileSchema, parseQueryString, parseCoordinatesAndFormat } from './schema';
import { ServiceHandler } from './ServiceHandler';
import { MatchServiceHandler } from './MatchServiceHandler';
import { NearestServiceHandler } from './NearestServiceHandler';
import { RouteServiceHandler } from './RouteServiceHandler';
import { TableServiceHandler } from './TableServiceHandler';
import { TripServiceHandler } from './TripServiceHandler';
import { Format } from './Format';

async function main() {
    const argv = await yargs(process.argv.slice(2)).options({
        ip: { type: 'string', default: '0.0.0.0', alias: 'i' },
        port: { type: 'number', default: 5000, alias: 'p' },
        workers: { type: 'number', alias: ['t', 'threads'], default: os.cpus().length },
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
        process.stdout.write(`v${OSRMVersion}\n`);
        return;
    }

    if (argv._.length == 0 && !argv.shared_memory) {
        // TODO: show usage
        return;
    }

    const osrm = new OSRMWrapper({
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

    const fastify = Fastify({
        logger: true,
        maxParamLength: Number.MAX_SAFE_INTEGER,
        rewriteUrl: (req) => {
            // https://github.com/fastify/fastify/issues/2487
            return req.url!.replace(/;/g, '%3B');
        },
        querystringParser: parseQueryString
    });
    await fastify.register(
      import('@fastify/compress')
    );



    async function processRequest(handler: ServiceHandler, request: FastifyRequest, reply: FastifyReply) {
        const { coordinatesAndFormat } = request.params as any;
        const query = request.query as any;

        try {
            const { format, coordinates } = parseCoordinatesAndFormat(coordinatesAndFormat);

            switch (format) {
            case Format.Json:
                reply.type('application/json').code(200);
                break;
            case Format.Flatbuffers:
                reply.type('application/x-flatbuffers;schema=osrm.engine.api.fbresult').code(200);
                break;
            }

            return  handler.handle(coordinates, query, format);
        } catch (e: any) {
            reply.code(400);

            // TODO: bindings do not return `message`, but put `code` into `message`
            return {
                code: e.message,
                message: e.message
            };
        }
    }

    fastify.get('/route/v1/:profile/:coordinatesAndFormat', { schema: routeSchema }, async (request, reply) => {
        return processRequest(new RouteServiceHandler(osrm), request, reply);
    });

    fastify.get('/nearest/v1/:profile/:coordinatesAndFormat', { schema: nearestSchema }, async (request, reply) => {
        return processRequest(new NearestServiceHandler(osrm), request, reply);
    });

    fastify.get('/table/v1/:profile/:coordinatesAndFormat', { schema: tableSchema }, async (request, reply) => {
        return processRequest(new TableServiceHandler(osrm), request, reply);
    });

    fastify.get('/match/v1/:profile/:coordinatesAndFormat', { schema: matchSchema }, async (request, reply) => {
        return processRequest(new MatchServiceHandler(osrm), request, reply);
    });

    fastify.get('/trip/v1/:profile/:coordinatesAndFormat', { schema: tripSchema }, async (request, reply) => {
        return processRequest(new TripServiceHandler(osrm), request, reply);
    });

    fastify.get('/tile/v1/:profile/tile(:x,:y,:zoom).mvt', { schema: tileSchema }, async (request, reply) => {
        const { x, y, zoom } = request.params as any;

        reply.type('application/x-protobuf').code(200);
        return osrm.tile([zoom, x, y]);
    });

    const start = async () => {
      try {
        await fastify.listen({ port: argv.port, host: argv.ip });
        process.stdout.write('running and waiting for requests\n');
      } catch (err) {
        fastify.log.error(err);
          process.exit(1); 
      }
    };

    const clusterWorkerSize = argv.workers;

    if (clusterWorkerSize > 1) {
      if (cluster.isMaster) {
          for (let i=0; i < clusterWorkerSize; i++) {
              cluster.fork();
          }
  
          cluster.on("exit", function(worker: any) {
              console.log("Worker", worker.id, " has exited.")
          })
      } else {
          start();
      }
    } else {
        start();
    }

}

main();

