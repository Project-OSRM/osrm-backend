import { EngineConfig, OSRM, RouteParams, RouteCallback } from 'osrm';


const cfg: EngineConfig = { 'path': 'nonexistent',
                            'algorithm': 'MLD' };

const osrm = new OSRM(cfg);

const depart = [ 13.414307, 52.521835 ];
const arrive = [ 13.402290, 52.523728 ];
const routeParams = { 'coordinates': [ depart, arrive ] };

const routeCallback: RouteCallback = (error, result) => {
  if (error) {
    console.log(`Error: ${error}`);
    return;
  }

  const waypoints = result!.waypoints;
  const routes = result!.routes;

  console.log(`Ok:\nwaypoints:\n${waypoints}\nroutes:\n${routes}`);
};

osrm.route(routeParams, routeCallback);
