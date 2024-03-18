import { ServiceHandler } from './ServiceHandler';

export class NearestServiceHandler extends ServiceHandler {
    protected buildServiceOptions(options: any, query: any): any {
        if (query.number !== undefined) {
            options.number = query.number;
        }
        return options;
    }

    protected async callOSRM(options: any): Promise<any> {
        return this.osrm.nearest(options);
    }
}