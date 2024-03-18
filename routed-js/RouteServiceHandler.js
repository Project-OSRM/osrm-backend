"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.RouteServiceHandler = void 0;
const ServiceHandler_1 = require("./ServiceHandler");
class RouteServiceHandler extends ServiceHandler_1.ServiceHandler {
    buildServiceOptions(options, query) {
        if (query.alternatives) {
            options.alternatives = query.alternatives;
        }
        if (query.approaches) {
            options.approaches = query.approaches;
        }
        if (query.waypoints) {
            options.waypoints = query.waypoints;
        }
        return options;
    }
    async callOSRM(options) {
        return this.osrm.route(options);
    }
}
exports.RouteServiceHandler = RouteServiceHandler;
