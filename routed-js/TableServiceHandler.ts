import { ServiceHandler } from './ServiceHandler';

export class TableServiceHandler extends ServiceHandler {
    protected buildServiceOptions(options: any, query: any): any {
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

    protected async callOSRM(options: any): Promise<any> {
        return this.osrm.table(options);
    }
}