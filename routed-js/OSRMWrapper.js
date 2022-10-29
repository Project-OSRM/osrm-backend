"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.OSRMWrapper = exports.version = void 0;
const util_1 = __importDefault(require("util"));
const OSRM = require('../lib/index.js');
exports.version = OSRM.version;
class OSRMWrapper {
    constructor(osrmOptions) {
        this.osrm = new OSRM(osrmOptions);
    }
    async tile(zxy) {
        return util_1.default.promisify(this.osrm.tile.bind(this.osrm))(zxy);
    }
    async route(options) {
        return util_1.default.promisify(this.osrm.route.bind(this.osrm))(options, { format: 'buffer' });
    }
    async nearest(options) {
        return util_1.default.promisify(this.osrm.nearest.bind(this.osrm))(options, { format: 'buffer' });
    }
    async table(options) {
        return util_1.default.promisify(this.osrm.table.bind(this.osrm))(options, { format: 'buffer' });
    }
    async trip(options) {
        return util_1.default.promisify(this.osrm.trip.bind(this.osrm))(options, { format: 'buffer' });
    }
    async match(options) {
        return util_1.default.promisify(this.osrm.match.bind(this.osrm))(options, { format: 'buffer' });
    }
}
exports.OSRMWrapper = OSRMWrapper;
