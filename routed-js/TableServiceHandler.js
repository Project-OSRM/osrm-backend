"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.TableServiceHandler = void 0;
const ServiceHandler_1 = require("./ServiceHandler");
class TableServiceHandler extends ServiceHandler_1.ServiceHandler {
    buildServiceOptions(options, query) {
        if (query.scale_factor) {
            options.scale_factor = query.scale_factor;
        }
        if (query.fallback_coordinate) {
            options.fallback_coordinate = query.fallback_coordinate;
        }
        if (query.fallback_speed) {
            options.fallback_speed = query.fallback_speed;
        }
        if (query.sources && query.sources !== 'all') {
            options.sources = query.sources;
        }
        if (query.destinations && query.destinations !== 'all') {
            options.destinations = query.destinations;
        }
        return options;
    }
    async callOSRM(options) {
        return this.osrm.table(options);
    }
}
exports.TableServiceHandler = TableServiceHandler;
