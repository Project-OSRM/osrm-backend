#include "osrm/engine_config.hpp"
#include "osrm/osrm.hpp"

#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/tile_parameters.hpp"
#include "osrm/trip_parameters.hpp"

#include <napi.h>
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
.. js:class:: OSRM(options)

   Creates an OSRM instance.  An OSRM instance requires a `.osrm.*` dataset (`.osrm.*`
   because it contains several files), which is prepared by the OSRM toolchain.  You can
   create such a `.osrm.*` dataset by running the OSRM binaries we ship in
   `node_modules/osrm/lib/binding_napi_v8/` and default profiles (e.g. for setting
   speeds and determining road types to route on) in `node_modules/osrm/profiles/`:

   .. code:: bash

      lib/binding_napi_v8/osrm-extract data.osm.pbf -p profiles/car.lua
      lib/binding_napi_v8/osrm-contract data.osrm

   See :doc:`/routed/index` for further details.

   Once you have generated a complete `network.osrm.*` dataset, you can instantiate an
   Object of this class, like this:

   .. code:: js

      const osrm = new OSRM('network.osrm');

   :param Object|String options: Either a String representing a path to the `.osrm` files
      or an Object containing one or more of the following properties:

      algorithm
         String: The algorithm to use for routing. Can be 'CH', or 'MLD'. Default is 'CH'.
         Make sure you prepared the dataset with the correct toolchain.

      shared_memory
         Boolean: Connects to the persistent shared memory datastore.  This requires you to
         run `osrm-datastore` prior to creating an `OSRM` object.

      dataset_name
         String: Connects to the persistent shared memory datastore defined
         by `--dataset_name` option when running `osrm-datastore`.
         This requires you to run `osrm-datastore --dataset_name` prior to creating an `OSRM` object.

      memory_file
         String: **DEPRECATED** Old behaviour: Path to a file on disk to store the memory using mmap.
         Current behaviour: setting this value is the same as setting `mmap_memory: true`.

      mmap_memory
         Boolean: Map on-disk files to virtual memory addresses (mmap), rather than loading into RAM.

      path
         String: The path to the `.osrm` files. This is mutually exclusive with
         setting `shared_memory` to true.

      disable_feature_dataset
         Array:  Disables a feature dataset from being loaded into memory if not needed.
         Options: `ROUTE_STEPS`, `ROUTE_GEOMETRY`.

      max_locations_trip
         Number: Max. locations supported in trip query (default: unlimited).

      max_locations_viaroute
         Number: Max. locations supported in viaroute query (default: unlimited).

      max_locations_distance_table
         Number: Max. locations supported in distance table query (default: unlimited).

      max_locations_map_matching
         Number: Max. locations supported in map-matching query (default: unlimited).

      max_radius_map_matching
         Number: Max. radius size supported in map matching query (default: 5).

      max_results_nearest
         Number: Max. results supported in nearest query (default: unlimited).

      max_alternatives
         Number: Max. number of alternatives supported in alternative routes query (default: 3).

      default_radius
         Number: Default radius for queries (default: unlimited).
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
   .. js:method:: route(options[, plugin_config], callback)

      Returns the fastest route between two or more coordinates while visiting the waypoints in order.

      :param Object options: Object containing one or more of the following properties:

         coordinates
            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]`
            values, in decimal degrees.

         bearings
            Array: Limits the search to segments with given bearing in degrees towards true
            north in clockwise direction.  Can be `null` or an Array of `[{value},{range}]`
            with `integer 0 .. 360,integer 0 .. 180`.

         radiuses
            Array: Limits the coordinate snapping to streets in the given radius in meters.
            Can be `null` (unlimited, default) or `double >= 0`.

         hints
            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         exclude
            Array: List of classes to avoid, order does not matter.

         generate_hints=true
            Boolean: Whether or not to return a hint which can be used in subsequent requests.

         alternatives=false
            Boolean: Search for alternative routes.

         alternatives=0
            Number: Search for up to this many alternative routes. *Please note that even if
            alternative routes are requested, a result cannot be guaranteed.*

         steps=false
            Boolean: Return route steps for each route leg.

         annotations=false
            Array|Boolean: An Array contaning strings of `duration`, `nodes`, `distance`,
            `weight`, `datasources`, `speed` or a boolean for enabling/disabling all.

         geometries=polyline
            String: Returned route geometry format (influences overview and per step). Can also be `geojson`.

         overview=simplified
            String: Add overview geometry either `full`, `simplified` according to highest
            zoom level it could be display on, or not at all (`false`).

         continue_straight
            Boolean: Forces the route to keep going straight at waypoints and don't do a uturn
            even if it would be faster. Default value depends on the profile.

         approaches
            Array: Restrict the direction on the road network at a waypoint, relative to the
            input coordinate.  Can be `null` (unrestricted, default), `curb` or `opposite`.

         waypoints
            Array: Indices to coordinates to treat as waypoints. If not supplied, all
            coordinates are waypoints.  Must include first and last coordinate index.

         format
            String: Which output format to use, either `json`, or
            `flatbuffers <https://github.com/Project-OSRM/osrm-backend/tree/master/include/engine/api/flatbuffers>`_.

         snapping
            String: Which edges can be snapped to, either `default`, or `any`.  `default` only
            snaps to edges marked by the profile as `is_startpoint`, `any` will allow snapping
            to any edge in the routing graph.

         skip_waypoints=false
            Boolean: Removes waypoints from the response. Waypoints are still calculated, but
            not serialized. Could be useful in case you are interested in some other part of
            response and do not want to transfer waste data.

      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:

      :returns: an Object containing an Array of :ref:`waypoint-object`
         representing all waypoints in order AND an Array of :ref:`route-object`
         ordered by descending recommendation rank.

      Example:

      .. code:: js

         const osrm = new OSRM("berlin-latest.osrm");
         osrm.route({coordinates: [[52.519930,13.438640], [52.513191,13.415852]]}, function(err, result) {
           if(err) throw err;
           console.log(result.waypoints); // Array of Waypoint objects representing all waypoints in order
           console.log(result.routes);    // Array of Route objects ordered by descending recommendation rank
         });

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
   .. js:method:: nearest(options[, plugin_config], callback)

      Snaps a coordinate to the street network and returns the nearest *n* matches.

      Note: `coordinates` in the general options only supports a single
      `{longitude},{latitude}` entry.

      :param Object options: Object containing one or more of the following properties:

         coordinates
            Array: The coordinates this request will use,
            coordinates as `[{lon},{lat}]` values, in decimal degrees.

         bearings
            Array: Limits the search to segments with given bearing in degrees
            towards true north in clockwise direction.  Can be `null` or an Array
            of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         radiuses
            Array: Limits the coordinate snapping to streets in the given radius in meters.
            Can be `null` (unlimited, default) or `double >= 0`.

         hints
            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         generate_hints=true
             Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         number=1
            Number: Number of nearest segments that should be returned.
            Must be an integer greater than or equal to `1`.

         approaches
            Array: Restrict the direction on the road network at a waypoint, relative to the input coordinate.
            Can be `null` (unrestricted, default), `curb` or `opposite`.

         format
            String: Which output format to use, either `json`, or
            `flatbuffers <https://github.com/Project-OSRM/osrm-backend/tree/master/include/engine/api/flatbuffers>`_.

         snapping
            String: Which edges can be snapped to, either `default`, or `any`.
            `default` only snaps to edges marked by the profile as `is_startpoint`,
            `any` will allow snapping to any edge in the routing graph.

      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: an Object containing the following properties:

         waypoints
            Array of :ref:`waypoint-object` sorted by distance to the input coordinate.
            Each object has an additional `distance` property, which is the distance
            in meters to the supplied input coordinate.

      Example:

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
           coordinates: [[13.388860,52.517037]],
           number: 3,
           bearings: [[0,20]]
         };
         osrm.nearest(options, function(err, response) {
           console.log(response.waypoints); // Array of Waypoint objects
         });

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
   .. js:method:: table(options[, plugin_config], callback)

      Computes duration table for the given locations. Allows for both symmetric and
      asymmetric tables.  Optionally returns distance table.

      :param Object options: Object containing one or more of the following properties:

         coordinates
            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.

         bearings
            Array: Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
            Can be `null` or an Array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         radiuses
            Array: Limits the coordinate snapping to streets in the given radius in meters.
            Can be `null` (unlimited, default) or `double >= 0`.

         hints
            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         generate_hints=true
            Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         sources
            Array: An Array of `index` elements (`0 <= integer < #coordinates`) to use
            location with given index as source. Default is to use all.

         destinations
            Array: An Array of `index` elements (`0 <= integer < #coordinates`) to use
            location with given index as destination. Default is to use all.

         approaches
            Array: Restrict the direction on the road network at a waypoint, relative to the input coordinate.
            Can be `null` (unrestricted, default), `curb` or `opposite`.

         fallback_speed
            Number: Replace `null` responses in result with as-the-crow-flies estimates based on `fallback_speed`.
            Value is in metres/second.

         fallback_coordinate
            String: Either `input` (default) or `snapped`.  If using a `fallback_speed`,
            use either the user-supplied coordinate (`input`), or the snapped coordinate (`snapped`)
            for calculating the as-the-crow-flies distance between two points.

         scale_factor
            Number: Multiply the table duration values in the table by this number
            for more controlled input into a route optimization solver.

         snapping
            String: Which edges can be snapped to, either `default`, or `any`.
            `default` only snaps to edges marked by the profile as `is_startpoint`,
            `any` will allow snapping to any edge in the routing graph.

         annotations
            Array: Return the requested table or tables in response. Can be either of
            `['duration']` (return the duration matrix, default),
            `['distance']` (return the distance matrix), or
            `['duration', 'distance']` (return both the duration matrix and the distance matrix).

      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: an Object containing the following properties:

         durations
            Array of Arrays that stores the matrix in row-major order.
            `durations[i][j]` gives the travel time from the i-th waypoint to the j-th waypoint.
            Values given in seconds.

         distances
            Array of Arrays that stores the matrix in row-major order.
            `distances[i][j]` gives the distance from the i-th waypoint to the j-th waypoint.
            Values given in meters.

         sources
            Array of :ref:`waypoint-object` describing all sources in order.

         destinations
            Array of :ref:`waypoint-object` describing all destinations in order.

         fallback_speed_cells (optional)
            if `fallback_speed` was given, this will be an Array of Arrays of `row, column` values,
            indicating which cells contain estimated values.

      Example:

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
           coordinates: [
             [13.388860,52.517037],
             [13.397634,52.529407],
             [13.428555,52.523219]
           ]
         };
         osrm.table(options, function(err, response) {
           console.log(response.durations);    // Array of arrays, matrix in row-major order
           console.log(response.distances);    // Array of arrays, matrix in row-major order
           console.log(response.sources);      // Array of Waypoint objects
           console.log(response.destinations); // Array of Waypoint objects
         });

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
   .. js:method:: tile(coordinates[, plugin_config], callback)

      This generates `Mapbox Vector Tiles <https://mapbox.com/vector-tiles>`_ that can be
      viewed with a vector-tile capable slippy-map viewer. The tiles contain road
      geometries and metadata that can be used to examine the routing graph. The tiles are
      generated directly from the data in-memory, so are in sync with actual routing
      results, and let you examine which roads are actually routable, and what weights they
      have applied.

      :param Array coordinates:
             an Array consisting of `x`, `y`, and `z` values representing tile coordinates like
             `wiki.openstreetmap.org/wiki/Slippy_map_tilenames <https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames>`_
             and are supported by vector tile viewers like `Mapbox GL JS <https://www.mapbox.com/mapbox-gl-js/api/>`_.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: buffer containing a Protocol-Buffer-encoded vector tile.

      Example:

      .. code:: js

         const osrm = new OSRM('network.osrm');
         osrm.tile([0, 0, 0], function(err, response) {
           if (err) throw err;
           fs.writeFileSync('./tile.vector.pbf', response); // write the buffer to a file
         });
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
   .. js:method:: match(coordinates[, plugin_config], callback)

      Map matching matches given GPS points to the road network in the most plausible way.
      Please note the request might result multiple sub-traces. Large jumps in the
      timestamps (>60s) or improbable transitions lead to trace splits if a complete
      matching could not be found. The algorithm might not be able to match all points.
      Outliers are removed if they can not be matched successfully.

      :param Object options: Object containing one or more of the following:

         coordinates
            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.

         bearings
            Array: Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
            Can be `null` or an Array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         hints
            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         generate_hints=true
            Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         steps=false
            Boolean: Return route steps for each route.

         annotations=false
            Array|Boolean: An Array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`,
            `speed` or boolean for enabling/disabling all.

         geometries=polyline
            String: Returned route geometry format (influences overview and per step). Can also be `geojson`.

         overview=simplified
            String: Add overview geometry either `full`, `simplified` according to highest zoom level
            it could be display on, or not at all (`false`).

         timestamps
            Array<Number>: Timestamp of the input location (integers, UNIX-like timestamp).

         radiuses
            Array: Standard deviation of GPS precision used for map matching. If applicable use GPS accuracy.
            Can be `null` for default value `5` meters or `double >= 0`.

         gaps=split
            String: Allows the input track splitting based on huge timestamp gaps between points. Either `split` or `ignore`.

         tidy=false
            Boolean: Allows the input track modification to obtain better matching quality for noisy tracks.

         waypoints
            Array: Indices to coordinates to treat as waypoints.  If not supplied, all coordinates are waypoints.
            Must include first and last coordinate index.

         snapping
            String: Which edges can be snapped to, either `default`, or `any`. `default` only snaps to edges
            marked by the profile as `is_startpoint`, `any` will allow snapping to any edge in the routing graph.

      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:
      :returns: an Object containing the following properties:

         tracepoints
            Array of :ref:`waypoint-object` representing all points of the trace in order.
            If the trace point was omitted by map matching because it is an outlier, the entry will be null.
            Each :ref:`waypoint-object` has the following additional properties,

            1. `matchings_index`: Index to the :ref:`route-object` in matchings
               the sub-trace was matched to,
            2. `waypoint_index`: Index of the waypoint inside the matched route.
            3. `alternatives_count`: Number of probable alternative matchings for this trace point.
               A value of zero indicate that this point was matched unambiguously.
               Split the trace at these points for incremental map matching.

         matchings
            Array of :ref:`route-object` that assemble the trace. Each :ref:`route-object` has an additional `confidence` property,
            which is the confidence of the matching. float value between `0` and `1`. `1` is very confident that the matching is correct.

      Example:

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
             coordinates: [[13.393252,52.542648],[13.39478,52.543079],[13.397389,52.542107]],
             timestamps: [1424684612, 1424684616, 1424684620]
         };
         osrm.match(options, function(err, response) {
             if (err) throw err;
             console.log(response.tracepoints); // Array of Waypoint objects
             console.log(response.matchings);   // Array of Route objects
         });
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
   .. js:method:: trip(coordinates[, plugin_config], callback)

      The trip plugin solves the Traveling Salesman Problem using a greedy heuristic
      (farthest-insertion algorithm) for 10 or more waypoints and uses brute force for less
      than 10 waypoints. The returned path does not have to be the shortest path, as TSP is
      NP-hard it is only an approximation.

      Note that all input coordinates have to be connected for the trip service to work.
      Currently, not all combinations of `roundtrip`, `source` and `destination` are
      supported.  Right now, the following combinations are possible:

      ========= ====== =========== =========
      roundtrip source destination supported
      ========= ====== =========== =========
      true      first  last        **yes**
      true      first  any         **yes**
      true      any    last        **yes**
      true      any    any         **yes**
      false     first  last        **yes**
      false     first  any         no
      false     any    last        no
      false     any    any         no
      ========= ====== =========== =========

      :param Object options: An Object containing one or more of the following properties:

         coordinates
            Array: The coordinates this request will use, coordinates as `[{lon},{lat}]` values, in decimal degrees.

         bearings
            Array: Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
            Can be `null` or an Array of `[{value},{range}]` with `integer 0 .. 360,integer 0 .. 180`.

         radiuses
            Array: Limits the coordinate snapping to streets in the given radius in meters.
            Can be `double >= 0` or `null` (unlimited, default).

         hints
            Array: Hints for the coordinate snapping. Array of base64 encoded strings.

         generate_hints=true
            Boolean: Whether or not adds a Hint to the response which can be used in subsequent requests.

         steps=false
            Boolean: Return route steps for each route.

         annotations=false
            Array|Boolean: An Array with strings of `duration`, `nodes`, `distance`, `weight`, `datasources`,
            `speed` or boolean for enabling/disabling all.

         geometries=polyline
            String: Returned route geometry format (influences overview and per step). Can also be `geojson`.

         overview=simplified
            String: Add overview geometry either `full`, `simplified`

         roundtrip=true
            Boolean: Return route is a roundtrip.

         source=any
            String: Return route starts at `any` or `first` coordinate.

         destination=an
            String: Return route ends at `any` or `last` coordinate.

         approaches
            Array: Restrict the direction on the road network at a waypoint, relative to the input coordinate.
            Can be `null` (unrestricted, default), `curb` or `opposite`.

         snapping
            String: Which edges can be snapped to, either `default`, or `any`.
            `default` only snaps to edges marked by the profile as `is_startpoint`,
            `any` will allow snapping to any edge in the routing graph.
      :param Object plugin_config: Plugin configuration. See: :js:class:`node.plugin_config`
      :param Function callback:

      :returns: an Object containing the following properties:

         waypoints
            Array of :ref:`waypoint-object` representing all waypoints in input order.
            Each Waypoint object has the following additional properties,

            1) `trips_index`: index to trips of the sub-trip the point was matched to, and
            2) `waypoint_index`: index of the point in the trip.

         trips
            Array of :ref:`route-object` that assemble the trace.

      .. code:: js

         const osrm = new OSRM('network.osrm');
         const options = {
           coordinates: [
             [13.36761474609375, 52.51663871100423],
             [13.374481201171875, 52.506191342034576]
           ],
           source: "first",
           destination: "last",
           roundtrip: false
         }
         osrm.trip(options, function(err, response) {
           if (err) throw err;
           console.log(response.waypoints); // Array of Waypoint objects
           console.log(response.trips);     // Array of Route objects
         });
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

// clang-format off
/**
.. js:class:: plugin_config

   All plugins support a second additional parameter to configure some NodeJS specific
   behaviours.

   format
      String: The format of the result object to various API calls. Valid options are `object`
      (default if `options.format` is `json`), which returns a standard Javascript object,
      as described above, and `buffer` (default if `options.format` is `flatbuffers`),
      which will return a NodeJS `Buffer <https://nodejs.org/api/buffer.html>`_ object,
      containing a JSON string or Flatbuffers object.
      The latter has the advantage that it can be immediately serialized to disk/sent over the
      network, and the generation of the string is performed outside the main NodeJS event loop.
      This option is ignored by the `tile` plugin. Also note that `options.format` set to
      `flatbuffers` cannot be used with `plugin_config.format` set to `object`. `json_buffer` is
      deprecated alias for `buffer`.

.. code:: js

   const osrm = new OSRM('network.osrm');
   const options = {
     coordinates: [
       [13.36761474609375, 52.51663871100423],
       [13.374481201171875, 52.506191342034576]
     ]
   };
   osrm.route(options, { format: "buffer" }, function(err, response) {
     if (err) throw err;
     console.log(response.toString("utf-8"));
   });
*/
// clang-format on

} // namespace node_osrm

Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
    return node_osrm::Engine::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)
