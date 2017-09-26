// Type definitions for node-osrm.
// Project: https://github.com/Project-OSRM/osrm-backend

// Impl. - type definition documentation, reference and examples can be found here:
// https://www.typescriptlang.org/docs/handbook/declaration-files/introduction.html
//
// Truth can only be found in include/nodejs/node_osrm_support.hpp

declare namespace osrm {

  /*
   * Open Source Routing Machine (OSRM).
   */
  export class OSRM {
    constructor(config: FilePath | EngineConfig);

    route(params: RouteParams, callback: RouteCallback): void;

    //  nearest(): void;
    //  table(): void;
    //  tile(): void;
    //  match(): void;
    //  trip(): void
  }

  /*
   * Path to a file.
   */
  export type FilePath = string;

  /*
   * Routing Engine Configuration.
   */
  export interface EngineConfig {
    path?: FilePath;
    shared_memory?: boolean;

    algorithm?: RoutingAlgorithm;

    max_locations_trip?: number;
    max_locations_viaroute?: number;
    max_locations_distance_table?: number;
    max_locations_map_matching?: number;
    max_results_nearest?: number;
    max_alternatives?: number;
  }

  /*
   * Implemented Routing Algorithms.
   *
   * CH:     Contraction Hierarchies
   * CoreCH: Contraction Hierarchies with a contraction threshold to speed up contraction
   * MLD:    Multi-Level Dijkstra
   */
  export type RoutingAlgorithm = 'CH' | 'CoreCH' | 'MLD';


  /*
   * Geographic coordinate on planet Earth's surface.
   */
  export type Coordinate = number[];


  /*
   * Parameters for the Route service.
   */
  export interface RouteParams {
    coordinates: Coordinate[];

  }


  /*
   * Callback function.
   */
  export type Callback<T> = (error: Error, result?: T) => void;

  export type RouteCallback = Callback<RouteResult>;


  /*
   *
   */
  export interface Waypoint {
    name: string;
    location: Coordinate;
    distance?: number;
    hint?: string;
  }

  /*
   *
   */
  export interface Route {
    distance: number;
    duration: number;
    weight: number;
    weight_name: number;
    //geometry?
    legs: RouteLeg[];
  }

  /*
   *
   */
  export interface RouteLeg {

  }

  /*
   * Callback results.
   */
  export interface RouteResult {
    waypoints: Waypoint[];
    routes:    Route[];
  }

} // namespace osrm

declare module 'osrm' {
  export = osrm;
}
