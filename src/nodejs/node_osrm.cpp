#include "osrm/engine_config.hpp"
#include "osrm/osrm.hpp"

#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/tile_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include <exception>
#include <sstream>
#include <type_traits>
#include <utility>

#include "nodejs/node_osrm.hpp"
#include "nodejs/node_osrm_support.hpp"

#include "util/json_renderer.hpp"

namespace node_osrm
{

Engine::Engine(osrm::EngineConfig &config) : Base(), this_(std::make_shared<osrm::OSRM>(config)) {}

Nan::Persistent<v8::Function> &Engine::constructor()
{
    static Nan::Persistent<v8::Function> init;
    return init;
}

NAN_MODULE_INIT(Engine::Init)
{
    const auto whoami = Nan::New("OSRM").ToLocalChecked();

    auto fnTp = Nan::New<v8::FunctionTemplate>(New);
    fnTp->InstanceTemplate()->SetInternalFieldCount(1);
    fnTp->SetClassName(whoami);

    SetPrototypeMethod(fnTp, "route", route);
    SetPrototypeMethod(fnTp, "nearest", nearest);
    SetPrototypeMethod(fnTp, "table", table);
    SetPrototypeMethod(fnTp, "tile", tile);
    SetPrototypeMethod(fnTp, "match", match);
    SetPrototypeMethod(fnTp, "trip", trip);

    const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();

    constructor().Reset(fn);

    Nan::Set(target, whoami, fn);
}

// clang-format off
/**
 * The `OSRM` method is the main constructor for creating an OSRM instance.
 * An OSRM instance requires a `.osrm` dataset, which is prepared by the OSRM toolchain.
 * You can create such a `.osrm` file by running the OSRM binaries we ship in `node_modules/osrm/lib/binding/` and default
 * profiles (e.g. for setting speeds and determining road types to route on) in `node_modules/osrm/profiles/`:
 *
 *     node_modules/osrm/lib/binding/osrm-extract data.osm.pbf -p node_modules/osrm/profiles/car.lua
 *     node_modules/osrm/lib/binding/osrm-contract data.osrm
 *
 * Consult the [osrm-backend](https://github.com/Project-OSRM/osrm-backend) documentation for further details.
 *
 * Once you have a complete `network.osrm` file, you can calculate routes in javascript with this object.
 *
 * ```javascript
 * var osrm = new OSRM('network.osrm');
 * ```
 *
 * @param {Object|String} [options={shared_memory: true}] Options for creating an OSRM object or string to the `.osrm` file.
 * @param {String} [options.algorithm] The algorithm to use for routing. Can be 'CH', 'CoreCH' or 'MLD'. Default is 'CH'.
 *        Make sure you prepared the dataset with the correct toolchain.
 * @param {Boolean} [options.shared_memory] Connects to the persistent shared memory datastore.
 *        This requires you to run `osrm-datastore` prior to creating an `OSRM` object.
 * @param {String} [options.path] The path to the `.osrm` files. This is mutually exclusive with setting {options.shared_memory} to true.
 * @param {String} [options.memory_file] Path to a file to store the memory using mmap.
 * @param {Number} [options.max_locations_trip] Max. locations supported in trip query (default: unlimited).
 * @param {Number} [options.max_locations_viaroute] Max. locations supported in viaroute query (default: unlimited).
 * @param {Number} [options.max_locations_distance_table] Max. locations supported in distance table query (default: unlimited).
 * @param {Number} [options.max_locations_map_matching] Max. locations supported in map-matching query (default: unlimited).
 * @param {Number} [options.max_radius_map_matching] Max. radius size supported in map matching query (default: 5).
 * @param {Number} [options.max_results_nearest] Max. results supported in nearest query (default: unlimited).
 * @param {Number} [options.max_alternatives] Max. number of alternatives supported in alternative routes query (default: 3).
 *
 * @class OSRM
 *
 */
// clang-format on
NAN_METHOD(Engine::New)
{
    if (info.IsConstructCall())
    {
        try
        {
            auto config = argumentsToEngineConfig(info);
            if (!config)
                return;

            auto *const self = new Engine(*config);
            self->Wrap(info.This());
        }
        catch (const std::exception &ex)
        {
            return Nan::ThrowTypeError(ex.what());
        }

        info.GetReturnValue().Set(info.This());
    }
    else
    {
        return Nan::ThrowTypeError(
            "Cannot call constructor as function, you need to use 'new' keyword");
    }
}

template <typename ParameterParser, typename ServiceMemFn>
inline void async(const Nan::FunctionCallbackInfo<v8::Value> &info,
                  ParameterParser argsToParams,
                  ServiceMemFn service,
                  bool requires_multiple_coordinates)
{
    auto params = argsToParams(info, requires_multiple_coordinates);
    if (!params)
        return;

    auto pluginParams = argumentsToPluginParameters(info);

    BOOST_ASSERT(params->IsValid());

    if (!info[info.Length() - 1]->IsFunction())
        return Nan::ThrowTypeError("last argument must be a callback function");

    auto *const self = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
    using ParamPtr = decltype(params);

    struct Worker final : Nan::AsyncWorker
    {
        using Base = Nan::AsyncWorker;

        Worker(std::shared_ptr<osrm::OSRM> osrm_,
               ParamPtr params_,
               ServiceMemFn service,
               Nan::Callback *callback,
               PluginParameters pluginParams_)
            : Base(callback), osrm{std::move(osrm_)}, service{std::move(service)},
              params{std::move(params_)}, pluginParams{std::move(pluginParams_)}
        {
        }

        void Execute() override try
        {
            osrm::engine::api::ResultT r;
            r = osrm::util::json::Object();
            const auto status = ((*osrm).*(service))(*params, r);
            auto json_result = r.get<osrm::json::Object>();
            ParseResult(status, json_result);
            if (pluginParams.renderJSONToBuffer)
            {
                std::ostringstream buf;
                osrm::util::json::render(buf, json_result);
                result = buf.str();
            }
            else
            {
                result = json_result;
            }
        }
        catch (const std::exception &e)
        {
            SetErrorMessage(e.what());
        }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;

            const constexpr auto argc = 2u;
            v8::Local<v8::Value> argv[argc] = {Nan::Null(), render(result)};

            callback->Call(argc, argv);
        }

        // Keeps the OSRM object alive even after shutdown until we're done with callback
        std::shared_ptr<osrm::OSRM> osrm;
        ServiceMemFn service;
        const ParamPtr params;
        const PluginParameters pluginParams;

        ObjectOrString result;
    };

    auto *callback = new Nan::Callback{info[info.Length() - 1].As<v8::Function>()};
    Nan::AsyncQueueWorker(
        new Worker{self->this_, std::move(params), service, callback, std::move(pluginParams)});
}

template <typename ParameterParser, typename ServiceMemFn>
inline void asyncForTiles(const Nan::FunctionCallbackInfo<v8::Value> &info,
                          ParameterParser argsToParams,
                          ServiceMemFn service,
                          bool requires_multiple_coordinates)
{
    auto params = argsToParams(info, requires_multiple_coordinates);
    if (!params)
        return;

    auto pluginParams = argumentsToPluginParameters(info);

    BOOST_ASSERT(params->IsValid());

    if (!info[info.Length() - 1]->IsFunction())
        return Nan::ThrowTypeError("last argument must be a callback function");

    auto *const self = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
    using ParamPtr = decltype(params);

    struct Worker final : Nan::AsyncWorker
    {
        using Base = Nan::AsyncWorker;

        Worker(std::shared_ptr<osrm::OSRM> osrm_,
               ParamPtr params_,
               ServiceMemFn service,
               Nan::Callback *callback,
               PluginParameters pluginParams_)
            : Base(callback), osrm{std::move(osrm_)}, service{std::move(service)},
              params{std::move(params_)}, pluginParams{std::move(pluginParams_)}
        {
        }

        void Execute() override try
        {
            result = std::string();
            const auto status = ((*osrm).*(service))(*params, result);
            auto str_result = result.get<std::string>();
            ParseResult(status, str_result);
        }
        catch (const std::exception &e)
        {
            SetErrorMessage(e.what());
        }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;

            const constexpr auto argc = 2u;
            auto str_result = result.get<std::string>();
            v8::Local<v8::Value> argv[argc] = {Nan::Null(), render(str_result)};

            callback->Call(argc, argv);
        }

        // Keeps the OSRM object alive even after shutdown until we're done with callback
        std::shared_ptr<osrm::OSRM> osrm;
        ServiceMemFn service;
        const ParamPtr params;
        const PluginParameters pluginParams;

        osrm::engine::api::ResultT result;
    };

    auto *callback = new Nan::Callback{info[info.Length() - 1].As<v8::Function>()};
    Nan::AsyncQueueWorker(
        new Worker{self->this_, std::move(params), service, callback, std::move(pluginParams)});
}

// clang-format off
/**
 * Returns the fastest route between two or more coordinates while visiting the waypoints in order.
 *
 * @name route
 * @memberof OSRM
 * @param {Object} options Object literal containing parameters for the route query.
 * @param {Array} [options.coordinates] The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.
 * @param {Array} [options.bearings] Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
 *                                   Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.
 * @param {Array} [options.radiuses] Limits the coordinate snapping to streets in the given radius in meters. Can be `null` (unlimited, default) or `double >= 0`.
 * @param {Array} [options.hints] Hints for the coordinate snapping. Array of base64 encoded strings.
 * @param {Boolean} [options.alternatives=false] Search for alternative routes.
 * @param {Number} [options.alternatives=0] Search for up to this many alternative routes.
 * *Please note that even if alternative routes are requested, a result cannot be guaranteed.*
 * @param {Boolean} [options.steps=false] Return route steps for each route leg.
 * @param {Array|Boolean} [options.annotations=false] An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed` or boolean for enabling/disabling all.
 * @param {String} [options.geometries=polyline] Returned route geometry format (influences overview and per step). Can also be `geojson`.
 * @param {String} [options.overview=simplified] Add overview geometry either `full`, `simplified` according to highest zoom level it could be display on, or not at all (`false`).
 * @param {Boolean} [options.continue_straight] Forces the route to keep going straight at waypoints and don't do a uturn even if it would be faster. Default value depends on the profile.
 * @param {Array} [options.approaches] Keep waypoints on curb side. Can be `null` (unrestricted, default) or `curb`.
 *                  `null`/`true`/`false`
 * @param {Function} callback
 *
 * @returns {Object} An array of [Waypoint](#waypoint) objects representing all waypoints in order AND an array of [`Route`](#route) objects ordered by descending recommendation rank.
 *
 * @example
 * var osrm = new OSRM("berlin-latest.osrm");
 * osrm.route({coordinates: [[52.519930,13.438640], [52.513191,13.415852]]}, function(err, result) {
 *   if(err) throw err;
 *   console.log(result.waypoints); // array of Waypoint objects representing all waypoints in order
 *   console.log(result.routes); // array of Route objects ordered by descending recommendation rank
 * });
 */
// clang-format on
NAN_METHOD(Engine::route) //
{
    async(info, &argumentsToRouteParameter, &osrm::OSRM::Route, true);
}

// clang-format off
/**
 * Snaps a coordinate to the street network and returns the nearest n matches.
 *
 * Note: `coordinates` in the general options only supports a single `{longitude},{latitude}` entry.
 *
 * @name nearest
 * @memberof OSRM
 * @param {Object} options - Object literal containing parameters for the nearest query.
 * @param {Array} [options.coordinates] The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.
 * @param {Array} [options.bearings] Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
 *                                   Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.
 * @param {Array} [options.radiuses] Limits the coordinate snapping to streets in the given radius in meters. Can be `null` (unlimited, default) or `double >= 0`.
 * @param {Array} [options.hints] Hints for the coordinate snapping. Array of base64 encoded strings.
 * @param {Number} [options.number=1] Number of nearest segments that should be returned.
 * Must be an integer greater than or equal to `1`.
 * @param {Array} [options.approaches] Keep waypoints on curb side. Can be `null` (unrestricted, default) or `curb`.
 * @param {Function} callback
 *
 * @returns {Object} containing `waypoints`.
 * **`waypoints`**: array of [`Ẁaypoint`](#waypoint) objects sorted by distance to the input coordinate.
 *                  Each object has an additional `distance` property, which is the distance in meters to the supplied input coordinate.
 *
 * @example
 * var osrm = new OSRM('network.osrm');
 * var options = {
 *   coordinates: [[13.388860,52.517037]],
 *   number: 3,
 *   bearings: [[0,20]]
 * };
 * osrm.nearest(options, function(err, response) {
 *   console.log(response.waypoints); // array of Waypoint objects
 * });
 */
// clang-format on
NAN_METHOD(Engine::nearest) //
{
    async(info, &argumentsToNearestParameter, &osrm::OSRM::Nearest, false);
}

// clang-format off
/**
 * Computes duration and distance tables for the given locations. Allows for both symmetric and asymmetric
 * tables.
 *
 * @name table
 * @memberof OSRM
 * @param {Object} options - Object literal containing parameters for the table query.
 * @param {Array} [options.coordinates] The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.
 * @param {Array} [options.bearings] Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
 *                                   Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.
 * @param {Array} [options.radiuses] Limits the coordinate snapping to streets in the given radius in meters. Can be `null` (unlimited, default) or `double >= 0`.
 * @param {Array} [options.hints] Hints for the coordinate snapping. Array of base64 encoded strings.
 * @param {Array} [options.sources] An array of `index` elements (`0 <= integer < #coordinates`) to use
 *                                  location with given index as source. Default is to use all.
 * @param {Array} [options.destinations] An array of `index` elements (`0 <= integer < #coordinates`) to use location with given index as destination. Default is to use all.
 * @param {Array} [options.approaches] Keep waypoints on curb side. Can be `null` (unrestricted, default) or `curb`.
 * @param {Array} [options.annotations] An array of the table types to return. Values can be `duration` or `distance` or both. If no annotations parameter is added, the default is to return the `durations` table. If `annotations=distance` or `annotations=duration,distance` is requested when running a MLD router, a `NotImplemented` error will be returned.

 * @param {Function} callback
 *
 * @returns {Object} containing `durations`, `distances`, `sources`, and `destinations`.
 * **`durations`**: array of arrays that stores the matrix in row-major order. `durations[i][j]` gives the travel time from the i-th waypoint to the j-th waypoint.
 *                  Values are given in seconds.
 * **`distances`**: array of arrays that stores the matrix in row-major order. `distances[i][j]` gives the travel time from the i-th waypoint to the j-th waypoint.
 *                  Values are given in meters. Note that computing the `distances` table is currently only implemented for CH. If `annotations=distance` or
 *                  `annotations=duration,distance` is requested when running a MLD router, a `NotImplemented` error will be returned.
 * **`sources`**: array of [`Ẁaypoint`](#waypoint) objects describing all sources in order.
 * **`destinations`**: array of [`Ẁaypoint`](#waypoint) objects describing all destinations in order.
 *
 * @example
 * var osrm = new OSRM('network.osrm');
 * var options = {
 *   coordinates: [
 *     [13.388860,52.517037],
 *     [13.397634,52.529407],
 *     [13.428555,52.523219]
 *   ]
 * };
 * osrm.table(options, function(err, response) {
 *   console.log(response.durations); // array of arrays, matrix in row-major order
 *   console.log(response.distances); // array of arrays, matrix in row-major order (currently only implemented for CH router)
 *   console.log(response.sources); // array of Waypoint objects
 *   console.log(response.destinations); // array of Waypoint objects
 * });
 */
// clang-format on
NAN_METHOD(Engine::table) //
{
    async(info, &argumentsToTableParameter, &osrm::OSRM::Table, true);
}

// clang-format off
/**
 * This generates [Mapbox Vector Tiles](https://mapbox.com/vector-tiles) that can be viewed with a
 * vector-tile capable slippy-map viewer. The tiles contain road geometries and metadata that can
 * be used to examine the routing graph. The tiles are generated directly from the data in-memory,
 * so are in sync with actual routing results, and let you examine which roads are actually
 * routable,
 * and what weights they have applied.
 *
 * @name tile
 * @memberof OSRM
 * @param {Array} ZXY - an array consisting of `x`, `y`, and `z` values representing tile coordinates like
 *        [wiki.openstreetmap.org/wiki/Slippy_map_tilenames](https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames)
 *        and are supported by vector tile viewers like [Mapbox GL JS](https://www.mapbox.com/mapbox-gl-js/api/).
 * @param {Function} callback
 *
 * @returns {Buffer} contains a Protocol Buffer encoded vector tile.
 *
 * @example
 * var osrm = new OSRM('network.osrm');
 * osrm.tile([0, 0, 0], function(err, response) {
 *   if (err) throw err;
 *   fs.writeFileSync('./tile.vector.pbf', response); // write the buffer to a file
 * });
 */
// clang-format on
NAN_METHOD(Engine::tile)
{
    asyncForTiles(info, &argumentsToTileParameters, &osrm::OSRM::Tile, {/*unused*/});
}

// clang-format off
/**
 * Map matching matches given GPS points to the road network in the most plausible way.
 * Please note the request might result multiple sub-traces. Large jumps in the timestamps
 * (>60s) or improbable transitions lead to trace splits if a complete matching could
 * not be found. The algorithm might not be able to match all points. Outliers are removed
 * if they can not be matched successfully.
 *
 * @name match
 * @memberof OSRM
 * @param {Object} options - Object literal containing parameters for the match query.
 * @param {Array} [options.coordinates] The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.
 * @param {Array} [options.bearings] Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
 *                                   Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.
 * @param {Array} [options.hints] Hints for the coordinate snapping. Array of base64 encoded strings.
 * @param {Boolean} [options.steps=false] Return route steps for each route.
 * @param {Array|Boolean} [options.annotations=false] An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed` or boolean for enabling/disabling all.
 * @param {String} [options.geometries=polyline] Returned route geometry format (influences overview and per step). Can also be `geojson`.
 * @param {String} [options.overview=simplified] Add overview geometry either `full`, `simplified` according to highest zoom level it could be display on, or not at all (`false`).
 * @param {Array<Number>} [options.timestamps] Timestamp of the input location (integers, UNIX-like timestamp).
 * @param {Array} [options.radiuses] Standard deviation of GPS precision used for map matching. If applicable use GPS accuracy. Can be `null` for default value `5` meters or `double >= 0`.
 * @param {String} [options.gaps] Allows the input track splitting based on huge timestamp gaps between points. Either `split` or `ignore` (optional, default `split`).
 * @param {Boolean} [options.tidy] Allows the input track modification to obtain better matching quality for noisy tracks (optional, default `false`).
 *
 * @param {Function} callback
 *
 * @returns {Object} containing `tracepoints` and `matchings`.
 * **`tracepoints`** Array of [`Ẁaypoint`](#waypoint) objects representing all points of the trace in order.
 *                   If the trace point was ommited by map matching because it is an outlier, the entry will be null.
 *                   Each `Waypoint` object includes two additional properties, 1) `matchings_index`: Index to the
 *                   [`Route`](#route) object in matchings the sub-trace was matched to, 2) `waypoint_index`: Index of
 *                   the waypoint inside the matched route.
 * **`matchings`** is an array of [`Route`](#route) objects that assemble the trace. Each `Route` object has an additional `confidence` property,
 *                 which is the confidence of the matching. float value between `0` and `1`. `1` is very confident that the matching is correct.
 *
 * @example
 * var osrm = new OSRM('network.osrm');
 * var options = {
 *     coordinates: [[13.393252,52.542648],[13.39478,52.543079],[13.397389,52.542107]],
 *     timestamps: [1424684612, 1424684616, 1424684620]
 * };
 * osrm.match(options, function(err, response) {
 *     if (err) throw err;
 *     console.log(response.tracepoints); // array of Waypoint objects
 *     console.log(response.matchings); // array of Route objects
 * });
 *
 */
// clang-format on
NAN_METHOD(Engine::match) //
{
    async(info, &argumentsToMatchParameter, &osrm::OSRM::Match, true);
}

// clang-format off
/**
 * The trip plugin solves the Traveling Salesman Problem using a greedy heuristic
 * (farthest-insertion algorithm) for 10 or * more waypoints and uses brute force for less than 10
 * waypoints. The returned path does not have to be the shortest path, * as TSP is NP-hard it is
 * only an approximation.
 *
 * Note that all input coordinates have to be connected for the trip service to work.
 * Currently, not all combinations of `roundtrip`, `source` and `destination` are supported.
 * Right now, the following combinations are possible:
 *
 * | roundtrip | source | destination | supported |
 * | :-- | :-- | :-- | :-- |
 * | true | first | last | **yes** |
 * | true | first | any | **yes** |
 * | true | any | last | **yes** |
 * | true | any | any | **yes** |
 * | false | first | last | **yes** |
 * | false | first | any | no |
 * | false | any | last | no |
 * | false | any | any | no |
 *
 * @name trip
 * @memberof OSRM
 * @param {Object} options - Object literal containing parameters for the trip query.
 * @param {Array} [options.coordinates] The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.
 * @param {Array} [options.bearings] Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
 *                                   Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.
 * @param {Array} [options.radiuses] Limits the coordinate snapping to streets in the given radius in meters. Can be `double >= 0` or `null` (unlimited, default).
 * @param {Array} [options.hints] Hints for the coordinate snapping. Array of base64 encoded strings.
 * @param {Boolean} [options.steps=false] Return route steps for each route.
 * @param {Array|Boolean} [options.annotations=false] An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed` or boolean for enabling/disabling all.
 * @param {String} [options.geometries=polyline] Returned route geometry format (influences overview and per step). Can also be `geojson`.
 * @param {String} [options.overview=simplified] Add overview geometry either `full`, `simplified`
 * @param {Function} callback
 * @param {Boolean} [options.roundtrip=true] Return route is a roundtrip.
 * @param {String} [options.source=any] Return route starts at `any` or `first` coordinate.
 * @param {String} [options.destination=any] Return route ends at `any` or `last` coordinate.
 * @param {Array} [options.approaches] Keep waypoints on curb side. Can be `null` (unrestricted, default) or `curb`.
 *
 * @returns {Object} containing `waypoints` and `trips`.
 * **`waypoints`**: an array of [`Waypoint`](#waypoint) objects representing all waypoints in input order.
 *                  Each Waypoint object has the following additional properties,
 *                  1) `trips_index`: index to trips of the sub-trip the point was matched to, and
 *                  2) `waypoint_index`: index of the point in the trip.
 * **`trips`**: an array of [`Route`](#route) objects that assemble the trace.
 *
 * @example
 * var osrm = new OSRM('network.osrm');
 * var options = {
 *   coordinates: [
 *     [13.36761474609375, 52.51663871100423],
 *     [13.374481201171875, 52.506191342034576]
 *   ],
 *   source: "first",
 *   destination: "last",
 *   roundtrip: false
 * }
 * osrm.trip(options, function(err, response) {
 *   if (err) throw err;
 *   console.log(response.waypoints); // array of Waypoint objects
 *   console.log(response.trips); // array of Route objects
 * });
 */
// clang-format on
NAN_METHOD(Engine::trip) //
{
    async(info, &argumentsToTripParameter, &osrm::OSRM::Trip, true);
}

/**
 * Responses
 * @class Responses
 */

/**
 * Represents a route through (potentially multiple) waypoints.
 *
 * @name Route
 * @memberof Responses
 *
 * @param {documentation} exteral in
 * [`osrm-backend`](../http.md#route)
 *
 */

/**
 * Represents a route between two waypoints.
 *
 * @name RouteLeg
 * @memberof Responses
 *
 * @param {documentation} exteral in
 * [`osrm-backend`](../http.md#routeleg)
 *
 */

/**
 * A step consists of a maneuver such as a turn or merge, followed by a distance of travel along a
 * single way to the subsequent step.
 *
 * @name RouteStep
 * @memberof Responses
 *
 * @param {documentation} exteral in
 * [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#routestep)
 *
 */

/**
 *
 * @name StepManeuver
 * @memberof Responses
 *
 * @param {documentation} exteral in
 * [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#stepmaneuver)
 *
 */

/**
 * Object used to describe waypoint on a route.
 *
 * @name Waypoint
 * @memberof Responses
 *
 * @param {documentation} exteral in
 * [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#waypoint)
 *
 */

} // namespace node_osrm
