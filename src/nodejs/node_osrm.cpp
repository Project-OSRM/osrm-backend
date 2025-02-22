

#include "osrm/engine_config.hpp"
#include "osrm/osrm.hpp"

#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/tile_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include <exception>
#include <napi.h>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "nodejs/node_osrm.hpp"
#include "nodejs/node_osrm_support.hpp"

#include "util/json_renderer.hpp"

namespace node_osrm
{
Napi::Object Engine::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(env,
                                      "OSRM",
                                      {
                                          InstanceMethod("route", &Engine::route),
                                          InstanceMethod("nearest", &Engine::nearest),
                                          InstanceMethod("table", &Engine::table),
                                          InstanceMethod("tile", &Engine::tile),
                                          InstanceMethod("match", &Engine::match),
                                          InstanceMethod("trip", &Engine::trip),
                                      });

    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("OSRM", func);
    return exports;
}

// clang-format off
/**
 * The `OSRM` method is the main constructor for creating an OSRM instance.
 * An OSRM instance requires a `.osrm.*` dataset(`.osrm.*` because it contains several files), which is prepared by the OSRM toolchain.
 * You can create such a `.osrm.*` dataset by running the OSRM binaries we ship in `node_modules/osrm/lib/binding/` and default
 * profiles (e.g. for setting speeds and determining road types to route on) in `node_modules/osrm/profiles/`:
 *
 *     node_modules/osrm/lib/binding/osrm-extract data.osm.pbf -p node_modules/osrm/profiles/car.lua
 *     node_modules/osrm/lib/binding/osrm-contract data.osrm
 *
 * Consult the [osrm-backend](https://github.com/Project-OSRM/osrm-backend) documentation for further details.
 *
 * Once you have a complete `network.osrm.*` dataset, you can calculate routes in javascript with this object.
 *
 * ```javascript
 * var osrm = new OSRM('network.osrm');
 * ```
 *
 * @param {Object|String} [options={shared_memory: true}] Options for creating an OSRM object or string to the `.osrm` file.
 * @param {String} [options.algorithm] The algorithm to use for routing. Can be 'CH', or 'MLD'. Default is 'CH'.
 *        Make sure you prepared the dataset with the correct toolchain.
 * @param {Boolean} [options.shared_memory] Connects to the persistent shared memory datastore.
 *        This requires you to run `osrm-datastore` prior to creating an `OSRM` object.
 * @param {String} [options.dataset_name] Connects to the persistent shared memory datastore defined by `--dataset_name` option when running `osrm-datastore`.
 *        This requires you to run `osrm-datastore --dataset_name` prior to creating an `OSRM` object.
 * @param {String} [options.memory_file] **DEPRECATED**
 *        Old behaviour: Path to a file on disk to store the memory using mmap.  Current behaviour: setting this value is the same as setting `mmap_memory: true`.
 * @param {Boolean} [options.mmap_memory] Map on-disk files to virtual memory addresses (mmap), rather than loading into RAM.
 * @param {String} [options.path] The path to the `.osrm` files. This is mutually exclusive with setting {options.shared_memory} to true.
 * @param {Array}  [options.disable_feature_dataset] Disables a feature dataset from being loaded into memory if not needed. Options: `ROUTE_STEPS`, `ROUTE_GEOMETRY`.
 * @param {Number} [options.max_locations_trip] Max. locations supported in trip query (default: unlimited).
 * @param {Number} [options.max_locations_viaroute] Max. locations supported in viaroute query (default: unlimited).
 * @param {Number} [options.max_locations_distance_table] Max. locations supported in distance table query (default: unlimited).
 * @param {Number} [options.max_locations_map_matching] Max. locations supported in map-matching query (default: unlimited).
 * @param {Number} [options.max_radius_map_matching] Max. radius size supported in map matching query (default: 5).
 * @param {Number} [options.max_results_nearest] Max. results supported in nearest query (default: unlimited).
 * @param {Number} [options.max_alternatives] Max. number of alternatives supported in alternative routes query (default: 3).
 * @param {Number} [options.default_radius] Default radius for queries (default: unlimited).
 *
 * @class OSRM
 *
 */
// clang-format on
Engine::Engine(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Engine>(info)
{

    try
    {
        auto config = argumentsToEngineConfig(info);
        if (!config)
            return;

        this_ = std::make_shared<osrm::OSRM>(*config);
    }
    catch (const std::exception &ex)
    {
        ThrowTypeError(info.Env(), ex.what());
    }
}

template <typename ParameterParser, typename ServiceMemFn>
inline void async(const Napi::CallbackInfo &info,
                  ParameterParser argsToParams,
                  ServiceMemFn service,
                  bool requires_multiple_coordinates)
{
    auto params = argsToParams(info, requires_multiple_coordinates);
    if (!params)
        return;
    auto pluginParams = argumentsToPluginParameters(info, params->format);

    BOOST_ASSERT(params->IsValid());

    if (!info[info.Length() - 1].IsFunction())
        return ThrowTypeError(info.Env(), "last argument must be a callback function");

    auto *const self = Napi::ObjectWrap<Engine>::Unwrap(info.This().As<Napi::Object>());
    using ParamPtr = decltype(params);

    struct Worker final : Napi::AsyncWorker
    {
        Worker(std::shared_ptr<osrm::OSRM> osrm_,
               ParamPtr params_,
               ServiceMemFn service,
               Napi::Function callback,
               PluginParameters pluginParams_)
            : Napi::AsyncWorker(callback), osrm{std::move(osrm_)}, service{std::move(service)},
              params{std::move(params_)}, pluginParams{std::move(pluginParams_)}
        {
        }

        void Execute() override
        try
        {
            switch (
                params->format.value_or(osrm::engine::api::BaseParameters::OutputFormatType::JSON))
            {
            case osrm::engine::api::BaseParameters::OutputFormatType::JSON:
            {
                osrm::engine::api::ResultT r;
                r = osrm::util::json::Object();
                const auto status = ((*osrm).*(service))(*params, r);
                auto &json_result = std::get<osrm::json::Object>(r);
                ParseResult(status, json_result);
                if (pluginParams.renderToBuffer)
                {
                    std::string json_string;
                    osrm::util::json::render(json_string, json_result);
                    result = std::move(json_string);
                }
                else
                {
                    result = std::move(json_result);
                }
            }
            break;
            case osrm::engine::api::BaseParameters::OutputFormatType::FLATBUFFERS:
            {
                osrm::engine::api::ResultT r = flatbuffers::FlatBufferBuilder();
                const auto status = ((*osrm).*(service))(*params, r);
                const auto &fbs_result = std::get<flatbuffers::FlatBufferBuilder>(r);
                ParseResult(status, fbs_result);
                BOOST_ASSERT(pluginParams.renderToBuffer);
                std::string result_str(
                    reinterpret_cast<const char *>(fbs_result.GetBufferPointer()),
                    fbs_result.GetSize());
                result = std::move(result_str);
            }
            break;
            }
        }
        catch (const std::exception &e)
        {
            SetError(e.what());
        }

        void OnOK() override
        {
            Napi::HandleScope scope{Env()};

            Callback().Call({Env().Null(), render(Env(), result)});
        }

        // Keeps the OSRM object alive even after shutdown until we're done with callback
        std::shared_ptr<osrm::OSRM> osrm;
        ServiceMemFn service;
        const ParamPtr params;
        const PluginParameters pluginParams;

        ObjectOrString result;
    };

    Napi::Function callback = info[info.Length() - 1].As<Napi::Function>();
    auto worker =
        new Worker(self->this_, std::move(params), service, callback, std::move(pluginParams));
    worker->Queue();
}

template <typename ParameterParser, typename ServiceMemFn>
inline void asyncForTiles(const Napi::CallbackInfo &info,
                          ParameterParser argsToParams,
                          ServiceMemFn service,
                          bool requires_multiple_coordinates)
{
    auto params = argsToParams(info, requires_multiple_coordinates);
    if (!params)
        return;

    auto pluginParams = argumentsToPluginParameters(info);

    BOOST_ASSERT(params->IsValid());

    if (!info[info.Length() - 1].IsFunction())
        return ThrowTypeError(info.Env(), "last argument must be a callback function");

    auto *const self = Napi::ObjectWrap<Engine>::Unwrap(info.This().As<Napi::Object>());
    using ParamPtr = decltype(params);

    struct Worker final : Napi::AsyncWorker
    {
        Worker(std::shared_ptr<osrm::OSRM> osrm_,
               ParamPtr params_,
               ServiceMemFn service,
               Napi::Function callback,
               PluginParameters pluginParams_)
            : Napi::AsyncWorker(callback), osrm{std::move(osrm_)}, service{std::move(service)},
              params{std::move(params_)}, pluginParams{std::move(pluginParams_)}
        {
        }

        void Execute() override
        try
        {
            result = std::string();
            const auto status = ((*osrm).*(service))(*params, result);
            auto str_result = std::get<std::string>(result);
            ParseResult(status, str_result);
        }
        catch (const std::exception &e)
        {
            SetError(e.what());
        }

        void OnOK() override
        {
            Napi::HandleScope scope{Env()};

            Callback().Call({Env().Null(), render(Env(), std::get<std::string>(result))});
        }

        // Keeps the OSRM object alive even after shutdown until we're done with callback
        std::shared_ptr<osrm::OSRM> osrm;
        ServiceMemFn service;
        const ParamPtr params;
        const PluginParameters pluginParams;

        osrm::engine::api::ResultT result;
    };

    Napi::Function callback = info[info.Length() - 1].As<Napi::Function>();
    auto worker =
        new Worker(self->this_, std::move(params), service, callback, std::move(pluginParams));
    worker->Queue();
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
 * @param {Array} [options.exclude] List of classes to avoid, order does not matter.
 * @param {Boolean} [options.generate_hints=true]  Whether or not adds a Hint to the response which can be used in subsequent requests.
 * @param {Boolean} [options.alternatives=false] Search for alternative routes.
 * @param {Number} [options.alternatives=0] Search for up to this many alternative routes.
 * *Please note that even if alternative routes are requested, a result cannot be guaranteed.*
 * @param {Boolean} [options.steps=false] Return route steps for each route leg.
 * @param {Array|Boolean} [options.annotations=false] An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed` or boolean for enabling/disabling all.
 * @param {String} [options.geometries=polyline] Returned route geometry format (influences overview and per step). Can also be `geojson`.
 * @param {String} [options.overview=simplified] Add overview geometry either `full`, `simplified` according to highest zoom level it could be display on, or not at all (`false`).
 * @param {Boolean} [options.continue_straight] Forces the route to keep going straight at waypoints and don't do a uturn even if it would be faster. Default value depends on the profile.
 * @param {Array} [options.approaches] Restrict the direction on the road network at a waypoint, relative to the input coordinate. Can be `null` (unrestricted, default), `curb` or `opposite`.
 *                  `null`/`true`/`false`
 * @param {Array} [options.waypoints] Indices to coordinates to treat as waypoints. If not supplied, all coordinates are waypoints.  Must include first and last coordinate index.
 * @param {String} [options.format] Which output format to use, either `json`, or [`flatbuffers`](https://github.com/Project-OSRM/osrm-backend/tree/master/include/engine/api/flatbuffers).
 * @param {String} [options.snapping] Which edges can be snapped to, either `default`, or `any`.  `default` only snaps to edges marked by the profile as `is_startpoint`, `any` will allow snapping to any edge in the routing graph.
 * @param {Boolean} [options.skip_waypoints=false] Removes waypoints from the response. Waypoints are still calculated, but not serialized. Could be useful in case you are interested in some other part of response and do not want to transfer waste data.
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
Napi::Value Engine::route(const Napi::CallbackInfo &info)
{
    osrm::Status (osrm::OSRM::*route_fn)(const osrm::RouteParameters &params,
                                         osrm::engine::api::ResultT &result) const =
        &osrm::OSRM::Route;
    async(info, &argumentsToRouteParameter, route_fn, true);
    return info.Env().Undefined();
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
 * @param {Boolean} [options.generate_hints=true]  Whether or not adds a Hint to the response which can be used in subsequent requests.
 * @param {Number} [options.number=1] Number of nearest segments that should be returned.
 * Must be an integer greater than or equal to `1`.
 * @param {Array} [options.approaches] Restrict the direction on the road network at a waypoint, relative to the input coordinate. Can be `null` (unrestricted, default), `curb` or `opposite`.
 * @param {String} [options.format] Which output format to use, either `json`, or [`flatbuffers`](https://github.com/Project-OSRM/osrm-backend/tree/master/include/engine/api/flatbuffers).
 * @param {String} [options.snapping] Which edges can be snapped to, either `default`, or `any`.  `default` only snaps to edges marked by the profile as `is_startpoint`, `any` will allow snapping to any edge in the routing graph.
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
Napi::Value Engine::nearest(const Napi::CallbackInfo &info)
{
    osrm::Status (osrm::OSRM::*nearest_fn)(const osrm::NearestParameters &params,
                                           osrm::engine::api::ResultT &result) const =
        &osrm::OSRM::Nearest;
    async(info, &argumentsToNearestParameter, nearest_fn, false);
    return info.Env().Undefined();
}

// clang-format off
/**
 * Computes duration table for the given locations. Allows for both symmetric and asymmetric tables.
 * Optionally returns distance table.
 *
 * @name table
 * @memberof OSRM
 * @param {Object} options - Object literal containing parameters for the table query.
 * @param {Array} [options.coordinates] The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.
 * @param {Array} [options.bearings] Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
 *                                   Can be `null` or an array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.
 * @param {Array} [options.radiuses] Limits the coordinate snapping to streets in the given radius in meters. Can be `null` (unlimited, default) or `double >= 0`.
 * @param {Array} [options.hints] Hints for the coordinate snapping. Array of base64 encoded strings.
 * @param {Boolean} [options.generate_hints=true] Whether or not adds a Hint to the response which can be used in subsequent requests.
 * @param {Array} [options.sources] An array of `index` elements (`0 <= integer < #coordinates`) to use
 *                                  location with given index as source. Default is to use all.
 * @param {Array} [options.destinations] An array of `index` elements (`0 <= integer < #coordinates`) to use location with given index as destination. Default is to use all.
 * @param {Array} [options.approaches] Restrict the direction on the road network at a waypoint, relative to the input coordinate.. Can be `null` (unrestricted, default), `curb` or `opposite`.
 * @param {Number} [options.fallback_speed] Replace `null` responses in result with as-the-crow-flies estimates based on `fallback_speed`.  Value is in metres/second.
 * @param {String} [options.fallback_coordinate] Either `input` (default) or `snapped`.  If using a `fallback_speed`, use either the user-supplied coordinate (`input`), or the snapped coordinate (`snapped`) for calculating the as-the-crow-flies distance between two points.
 * @param {Number} [options.scale_factor] Multiply the table duration values in the table by this number for more controlled input into a route optimization solver.
 * @param {String} [options.snapping] Which edges can be snapped to, either `default`, or `any`.  `default` only snaps to edges marked by the profile as `is_startpoint`, `any` will allow snapping to any edge in the routing graph.
 * @param {Array} [options.annotations] Return the requested table or tables in response. Can be `['duration']` (return the duration matrix, default), `[distance']` (return the distance matrix), or `['duration', distance']` (return both the duration matrix and the distance matrix).
 * @param {Function} callback
 *
 * @returns {Object} containing `durations`, `distances`, `sources`, and `destinations`.
 * **`durations`**: array of arrays that stores the matrix in row-major order. `durations[i][j]` gives the travel time from the i-th waypoint to the j-th waypoint.
 *                  Values are given in seconds.
 * **`distances`**: array of arrays that stores the matrix in row-major order. `distances[i][j]` gives the travel time from the i-th waypoint to the j-th waypoint.
 *                  Values are given in meters.
 * **`sources`**: array of [`Ẁaypoint`](#waypoint) objects describing all sources in order.
 * **`destinations`**: array of [`Ẁaypoint`](#waypoint) objects describing all destinations in order.
 * **`fallback_speed_cells`**: (optional) if `fallback_speed` is used, will be an array of arrays of `row,column` values, indicating which cells contain estimated values.
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
 *   console.log(response.distances); // array of arrays, matrix in row-major order
 *   console.log(response.sources); // array of Waypoint objects
 *   console.log(response.destinations); // array of Waypoint objects
 * });
 */
// clang-format on
Napi::Value Engine::table(const Napi::CallbackInfo &info)
{
    osrm::Status (osrm::OSRM::*table_fn)(const osrm::TableParameters &params,
                                         osrm::engine::api::ResultT &result) const =
        &osrm::OSRM::Table;
    async(info, &argumentsToTableParameter, table_fn, true);
    return info.Env().Undefined();
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
Napi::Value Engine::tile(const Napi::CallbackInfo &info)
{
    osrm::Status (osrm::OSRM::*tile_fn)(const osrm::TileParameters &params,
                                        osrm::engine::api::ResultT &result) const =
        &osrm::OSRM::Tile;
    asyncForTiles(info, &argumentsToTileParameters, tile_fn, {/*unused*/});
    return info.Env().Undefined();
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
 * @param {Boolean} [options.generate_hints=true] Whether or not adds a Hint to the response which can be used in subsequent requests.
 * @param {Boolean} [options.steps=false] Return route steps for each route.
 * @param {Array|Boolean} [options.annotations=false] An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed` or boolean for enabling/disabling all.
 * @param {String} [options.geometries=polyline] Returned route geometry format (influences overview and per step). Can also be `geojson`.
 * @param {String} [options.overview=simplified] Add overview geometry either `full`, `simplified` according to highest zoom level it could be display on, or not at all (`false`).
 * @param {Array<Number>} [options.timestamps] Timestamp of the input location (integers, UNIX-like timestamp).
 * @param {Array} [options.radiuses] Standard deviation of GPS precision used for map matching. If applicable use GPS accuracy. Can be `null` for default value `5` meters or `double >= 0`.
 * @param {String} [options.gaps=split] Allows the input track splitting based on huge timestamp gaps between points. Either `split` or `ignore`.
 * @param {Boolean} [options.tidy=false] Allows the input track modification to obtain better matching quality for noisy tracks.
 * @param {Array} [options.waypoints] Indices to coordinates to treat as waypoints.  If not supplied, all coordinates are waypoints.  Must include first and last coordinate index.
 * @param {String} [options.snapping] Which edges can be snapped to, either `default`, or `any`. `default` only snaps to edges marked by the profile as `is_startpoint`, `any` will allow snapping to any edge in the routing graph.
 *
 * @param {Function} callback
 *
 * @returns {Object} containing `tracepoints` and `matchings`.
 * **`tracepoints`** Array of [`Ẁaypoint`](#waypoint) objects representing all points of the trace in order.
 *                   If the trace point was omitted by map matching because it is an outlier, the entry will be null.
 *                   Each `Waypoint` object has the following additional properties,
 *                   1) `matchings_index`: Index to the
 *                   [`Route`](#route) object in matchings the sub-trace was matched to,
 *                   2) `waypoint_index`: Index of
 *                   the waypoint inside the matched route.
 *                   3) `alternatives_count`: Number of probable alternative matchings for this trace point. A value of zero indicate that this point was matched unambiguously. Split the trace at these points for incremental map matching.
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
Napi::Value Engine::match(const Napi::CallbackInfo &info)
{
    osrm::Status (osrm::OSRM::*match_fn)(const osrm::MatchParameters &params,
                                         osrm::engine::api::ResultT &result) const =
        &osrm::OSRM::Match;
    async(info, &argumentsToMatchParameter, match_fn, true);
    return info.Env().Undefined();
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
 * @param {Boolean} [options.generate_hints=true] Whether or not adds a Hint to the response which can be used in subsequent requests.
 * @param {Boolean} [options.steps=false] Return route steps for each route.
 * @param {Array|Boolean} [options.annotations=false] An array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed` or boolean for enabling/disabling all.
 * @param {String} [options.geometries=polyline] Returned route geometry format (influences overview and per step). Can also be `geojson`.
 * @param {String} [options.overview=simplified] Add overview geometry either `full`, `simplified`
 * @param {Function} callback
 * @param {Boolean} [options.roundtrip=true] Return route is a roundtrip.
 * @param {String} [options.source=any] Return route starts at `any` or `first` coordinate.
 * @param {String} [options.destination=any] Return route ends at `any` or `last` coordinate.
 * @param {Array} [options.approaches] Restrict the direction on the road network at a waypoint, relative to the input coordinate. Can be `null` (unrestricted, default), `curb` or `opposite`.
 * @param {String} [options.snapping] Which edges can be snapped to, either `default`, or `any`. `default` only snaps to edges marked by the profile as `is_startpoint`, `any` will allow snapping to any edge in the routing graph.
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
Napi::Value Engine::trip(const Napi::CallbackInfo &info)
{
    osrm::Status (osrm::OSRM::*trip_fn)(const osrm::TripParameters &params,
                                        osrm::engine::api::ResultT &result) const =
        &osrm::OSRM::Trip;
    async(info, &argumentsToTripParameter, trip_fn, true);
    return info.Env().Undefined();
}

/**
 * All plugins support a second additional object that is available to configure some NodeJS
 * specific behaviours.
 *
 * @name Configuration
 * @param {Object} [plugin_config] - Object literal containing parameters for the trip query.
 * @param {String} [plugin_config.format] The format of the result object to various API calls.
 *                                        Valid options are `object` (default if `options.format` is
 * `json`), which returns a standard Javascript object, as described above, and `buffer`(default if
 * `options.format` is `flatbuffers`), which will return a NodeJS
 * **[Buffer](https://nodejs.org/api/buffer.html)** object, containing a JSON string or Flatbuffers
 * object. The latter has the advantage that it can be immediately serialized to disk/sent over the
 * network, and the generation of the string is performed outside the main NodeJS event loop.  This
 * option is ignored by the `tile` plugin. Also note that `options.format` set to `flatbuffers`
 * cannot be used with `plugin_config.format` set to `object`. `json_buffer` is deprecated alias for
 * `buffer`.
 *
 * @example
 * var osrm = new OSRM('network.osrm');
 * var options = {
 *   coordinates: [
 *     [13.36761474609375, 52.51663871100423],
 *     [13.374481201171875, 52.506191342034576]
 *   ]
 * };
 * osrm.route(options, { format: "buffer" }, function(err, response) {
 *   if (err) throw err;
 *   console.log(response.toString("utf-8"));
 * });
 */

/**
 * @class Responses
 */

/**
 * Represents a route through (potentially multiple) waypoints.
 *
 * @name Route
 * @memberof Responses
 *
 * @param {documentation} external in
 * [`osrm-backend`](../http.md#route-object)
 *
 */

/**
 * Represents a route between two waypoints.
 *
 * @name RouteLeg
 * @memberof Responses
 *
 * @param {documentation} external in
 * [`osrm-backend`](../http.md#routeleg-object)
 *
 */

/**
 * A step consists of a maneuver such as a turn or merge, followed by a distance of travel along a
 * single way to the subsequent step.
 *
 * @name RouteStep
 * @memberof Responses
 *
 * @param {documentation} external in
 * [`osrm-backend`](../http.md#routestep-object)
 *
 */

/**
 *
 * @name StepManeuver
 * @memberof Responses
 *
 * @param {documentation} external in
 * [`osrm-backend`](../http.md#stepmaneuver-object)
 *
 */

/**
 * Object used to describe waypoint on a route.
 *
 * @name Waypoint
 * @memberof Responses
 *
 * @param {documentation} external in
 * [`osrm-backend`](../http.md#waypoint-object)
 *
 */

} // namespace node_osrm

Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
    return node_osrm::Engine::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)
