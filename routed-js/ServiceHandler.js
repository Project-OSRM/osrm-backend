"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ServiceHandler = void 0;
class ServiceHandler {
    constructor(osrm) {
        this.osrm = osrm;
    }
    async handle(coordinates, query, format) {
        const options = this.build(coordinates, query, format);
        return this.callOSRM(options);
    }
    build(coordinates, query, format) {
        const options = this.buildBaseOptions(coordinates, query, format);
        return this.buildServiceOptions(options, query);
    }
    buildBaseOptions(coordinates, query, format) {
        const options = {
            coordinates: coordinates
        };
        this.handleCommonParams(query, options, format);
        return options;
    }
    handleCommonParams(query, options, format) {
        options.format = format;
        if (query.overview) {
            options.overview = query.overview;
        }
        if (query.geometries) {
            options.geometries = query.geometries;
        }
        if (query.steps) {
            options.steps = query.steps;
        }
        // TODO: annotations is per-service option
        if (query.annotations) {
            options.annotations = query.annotations;
        }
        if (query.exclude) {
            options.exclude = query.exclude;
        }
        if (query.snapping) {
            options.snapping = query.snapping;
        }
        if (query.radiuses) {
            options.radiuses = query.radiuses.map((r) => {
                if (r === 'unlimited') {
                    return null;
                }
                return r;
            });
        }
        if (query.bearings) {
            options.bearings = query.bearings.map((bearingWithRange) => {
                if (bearingWithRange.length == 0) {
                    return null;
                }
                return bearingWithRange;
            });
        }
        if (query.hints) {
            options.hints = query.hints;
        }
        if (query.generate_hints) {
            options.generate_hints = query.generate_hints;
        }
        if (query.skip_waypoints) {
            options.skip_waypoints = query.skip_waypoints;
        }
        if (query.continue_straight) {
            options.continue_straight = ['default', 'true'].includes(query.continue_straight);
        }
    }
}
exports.ServiceHandler = ServiceHandler;
