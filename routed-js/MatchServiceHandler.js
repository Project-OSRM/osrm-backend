"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.MatchServiceHandler = void 0;
const ServiceHandler_1 = require("./ServiceHandler");
class MatchServiceHandler extends ServiceHandler_1.ServiceHandler {
    buildServiceOptions(options, query) {
        if (query.timestamps) {
            options.timestamps = query.timestamps;
        }
        if (query.waypoints) {
            options.waypoints = query.waypoints;
        }
        if (query.gaps) {
            options.gaps = query.gaps;
        }
        if (query.tidy) {
            options.tidy = query.tidy;
        }
        return options;
    }
    async callOSRM(options) {
        return this.osrm.match(options);
    }
}
exports.MatchServiceHandler = MatchServiceHandler;
