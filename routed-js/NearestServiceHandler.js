"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.NearestServiceHandler = void 0;
const ServiceHandler_1 = require("./ServiceHandler");
class NearestServiceHandler extends ServiceHandler_1.ServiceHandler {
    buildServiceOptions(options, query) {
        if (query.number !== undefined) {
            options.number = query.number;
        }
        return options;
    }
    async callOSRM(options) {
        return this.osrm.nearest(options);
    }
}
exports.NearestServiceHandler = NearestServiceHandler;
