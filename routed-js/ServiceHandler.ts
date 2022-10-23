
import { Format } from './Format';
import { OSRMWrapper } from './OSRMWrapper';



export abstract class ServiceHandler {
    public constructor(protected readonly osrm: OSRMWrapper) { }
    public async handle(coordinates: [number, number][], query: any, format: Format): Promise<any> {
        const options = this.build(coordinates, query, format);
        return this.callOSRM(options);
    }



    private build(coordinates: [number, number][], query: any, format: Format): any {
        const options = this.buildBaseOptions(coordinates, query, format);
        return this.buildServiceOptions(options, query);
    }

    protected abstract buildServiceOptions(options: any, query: any): any;
    protected abstract callOSRM(options: any): Promise<any>;

    private buildBaseOptions(coordinates: [number, number][], query: any, format: Format): any {
        const options: any = {
            coordinates: coordinates
        };
        this.handleCommonParams(query, options, format);
        return options;
    }

    private handleCommonParams(query: any, options: any, format: Format) {
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
            options.radiuses = query.radiuses.map((r: string | 'unlimited') => {
                if (r === 'unlimited') {
                    return null;
                }
                return r;
            });
        }

        if (query.bearings) {
            options.bearings = query.bearings.map((bearingWithRange: number[]) => {
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
