"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.TripServiceHandler = void 0;
const ServiceHandler_1 = require("./ServiceHandler");
class TripServiceHandler extends ServiceHandler_1.ServiceHandler {
    buildServiceOptions(options, query) {
        if (query.roundtrip != null) {
            options.roundtrip = query.roundtrip;
        }
        if (query.source) {
            options.source = query.source;
        }
        if (query.destination) {
            options.destination = query.destination;
        }
        return options;
    }
    async callOSRM(options) {
        return this.osrm.trip(options);
    }
}
exports.TripServiceHandler = TripServiceHandler;
