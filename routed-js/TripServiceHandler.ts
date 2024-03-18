import { ServiceHandler } from './ServiceHandler';

export class TripServiceHandler extends ServiceHandler {
    protected buildServiceOptions(options: any, query: any): any {
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


    protected async callOSRM(options: any): Promise<any> {
        return this.osrm.trip(options);
    }
}
