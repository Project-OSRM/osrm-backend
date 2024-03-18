
import { ServiceHandler } from './ServiceHandler';

export class RouteServiceHandler extends ServiceHandler {
    protected buildServiceOptions(options: any, query: any): any {

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


    protected async callOSRM(options: any): Promise<any> {
        return this.osrm.route(options);
    }
}