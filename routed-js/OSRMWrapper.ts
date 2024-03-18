import util from 'util';
const OSRM = require('../lib/index.js');

export const version = OSRM.version;

export class OSRMWrapper {
    private readonly osrm: typeof OSRM;

    constructor(osrmOptions: any) {
        this.osrm = new OSRM(osrmOptions);
    }

    async tile(zxy: [number, number, number]): Promise<any> {
        return util.promisify(this.osrm.tile.bind(this.osrm))(zxy);
    }

    async route(options: any): Promise<any> {
        return util.promisify(this.osrm.route.bind(this.osrm))(options, {format: 'buffer'});
    }

    async nearest(options: any): Promise<any> {
        return util.promisify(this.osrm.nearest.bind(this.osrm))(options, {format: 'buffer'});
    }

    async table(options: any): Promise<any> {
        return util.promisify(this.osrm.table.bind(this.osrm))(options, {format: 'buffer'});
    }

    async trip(options: any): Promise<any> {
        return util.promisify(this.osrm.trip.bind(this.osrm))(options, {format: 'buffer'});
    }

    async match(options: any): Promise<any> {
        return util.promisify(this.osrm.match.bind(this.osrm))(options, {format: 'buffer'});
    }
}
