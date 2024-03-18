import { ServiceHandler } from './ServiceHandler';

export class MatchServiceHandler extends ServiceHandler {
    protected buildServiceOptions(options: any, query: any): any {

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

    protected async callOSRM(options: any): Promise<any> {
        return this.osrm.match(options);
    }
}