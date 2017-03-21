# OSRM

The `OSRM` method is the main constructor for creating an OSRM instance. An OSRM instance requires a `.osrm` network,
which is prepared by the OSRM Backend C++ library.

You can create such a `.osrm` file by running the OSRM binaries we ship in `node_modules/osrm/lib/binding/` and default
profiles (e.g. for setting speeds and determining road types to route on) in `node_modules/osrm/profiles/`:

    node_modules/osrm/lib/binding/osrm-extract data.osm.pbf -p node_modules/osrm/profiles/car.lua
    node_modules/osrm/lib/binding/osrm-contract data.osrm

Consult the [osrm-backend](https://github.com/Project-OSRM/osrm-backend) documentation or further details.

Once you have a complete `network.osrm` file, you can calculate networks in javascript with this library using the
methods below. To create an OSRM instance with your network you need to construct an instance like this:

```javascript
var osrm = new OSRM('network.osrm');
```

#### Methods

| Service                    | Description                                               |
| -------------------------- | --------------------------------------------------------- |
| [`osrm.route`](#route)     | shortest path between given coordinates                   |
| [`osrm.nearest`](#nearest) | returns the nearest street segment for a given coordinate |
| [`osrm.table`](#table)     | computes distance tables for given coordinates            |
| [`osrm.match`](#match)     | matches given coordinates to the road network             |
| [`osrm.trip`](#trip)       | Compute the shortest trip between given coordinates       |
| [`osrm.tile`](#tile)       | Return vector tiles containing debugging info             |

#### General Options

Each OSRM method (except for `OSRM.tile()`) has set of general options as well as unique options, outlined below.

| Option          | Values                                                  | Description                                                                                            | Format                                                                         |
| --------------- | ------------------------------------------------------- | ------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------ |
| coordinates     | `array` of `coordinate` elements: `[{coordinate}, ...]` | The coordinates this request will use.                                                                 | `array` with `[{lon},{lat}]` values, in decimal degrees                        |
| bearings        | `array` of `bearing` elements: `[{bearing}, ...]`       | Limits the search to segments with given bearing in degrees towards true north in clockwise direction. | `null` or `array` with `[{value},{range}]` `integer 0 .. 360,integer 0 .. 180` |
| radiuses        | `array` of `radius` elements: `[{radius}, ...]`         | Limits the search to given radius in meters.                                                           | `null` or `double >= 0` or `unlimited` (default)                               |
| hints           | `array` of `hint` elements: `[{hint}, ...]`             | Hint to derive position in street network.                                                             | Base64 `string`                                                                |
| generate\_hints | `true` (default) or `false`                             | Adds a Hint to the response which can be used in subsequent requests, see `hints` parameter.           | `Boolean`                                                                      |

## route

Returns the fastest route between two or more coordinates while visiting the waypoints in order.

**Parameters**

-   `options` **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** Object literal containing parameters for the route query.
    -   `options.alternatives` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)]** Search for alternative routes and return as well. _Please note that even if an alternative route is requested, a result cannot be guaranteed._ (optional, default `false`)
    -   `options.steps` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)]** Return route steps for each route leg. (optional, default `false`)
    -   `options.annotations` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)] or \[[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)>]** Return annotations for each route leg for duration, nodes, distance, weight, datasources and/or speed. Annotations can be `false` or `true` (no/full annotations) or an array of strings with `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed`. (optional, default `false`)
    -   `options.geometries` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Returned route geometry format (influences overview and per step). Can also be `geojson`. (optional, default `polyline`)
    -   `options.overview` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Add overview geometry either `full`, `simplified` according to highest zoom level it could be display on, or not at all (`false`). (optional, default `simplified`)
    -   `options.continue_straight` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)]** Forces the route to keep going straight at waypoints and don't do a uturn even if it would be faster. Default value depends on the profile. `null`/`true`/`false`
-   `callback` **[Function](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/function)** 

**Examples**

```javascript
var osrm = new OSRM("berlin-latest.osrm");
osrm.route({coordinates: [[13.438640,52.519930], [13.415852, 52.513191]]}, function(err, result) {
  if(err) throw err;
  console.log(result.waypoints); // array of Waypoint objects representing all waypoints in order
  console.log(result.routes); // array of Route objects ordered by descending recommendation rank
});
```

Returns **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** An array of [Waypoint](#waypoint) objects representing all waypoints in order AND an array of [`Route`](#route) objects ordered by descending recommendation rank.

## nearest

Snaps a coordinate to the street network and returns the nearest n matches.

Note: `coordinates` in the general options only supports a single `{longitude},{latitude}` entry.

**Parameters**

-   `options` **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** Object literal containing parameters for the nearest query.
    -   `options.number` **\[[Number](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number)]** Number of nearest segments that should be returned.
        Must be an integer greater than or equal to `1`. (optional, default `1`)
-   `callback` **[Function](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/function)** 

**Examples**

```javascript
var osrm = new OSRM('network.osrm');
var options = {
  coordinates: [[13.388860,52.517037]],
  number: 3,
  bearings: [[0,20]]
};
osrm.nearest(options, function(err, response) {
  console.log(response.waypoints); // array of Waypoint objects
});
```

Returns **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** containing `waypoints`.
**`waypoints`**: array of [`Ẁaypoint`](#waypoint) objects sorted by distance to the input coordinate.
Each object has an additional `distance` property, which is the distance in meters to the supplied
input coordinate.

## table

Computes duration tables for the given locations. Allows for both symmetric and asymmetric tables.

**Parameters**

-   `options` **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** Object literal containing parameters for the table query.
    -   `options.sources` **\[[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)]** An array of `index` elements (`0 <= integer < #coordinates`) to use
        location with given index as source. Default is to use all.
    -   `options.destinations` **\[[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)]** An array of `index` elements (`0 <= integer < #coordinates`) to use location with given index as destination. Default is to use all.
-   `callback` **[Function](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/function)** 

**Examples**

```javascript
var osrm = new OSRM('network.osrm');
var options = {
  coordinates: [
    [13.388860,52.517037],
    [13.397634,52.529407],
    [13.428555,52.523219]
  ]
};
osrm.table(options, function(err, response) {
  console.log(response.durations); // array of arrays, matrix in row-major order
  console.log(response.sources); // array of Waypoint objects
  console.log(response.destinations); // array of Waypoint objects
});
```

Returns **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** containing `durations`, `sources`, and `destinations`.
**`durations`**: array of arrays that stores the matrix in row-major order. `durations[i][j]`
gives the travel time from the i-th waypoint to the j-th waypoint. Values are given in seconds.
**`sources`**: array of [`Ẁaypoint`](#waypoint) objects describing all sources in order.
**`destinations`**: array of [`Ẁaypoint`](#waypoint) objects describing all destinations in order.

## tile

This generates [Mapbox Vector Tiles](https://mapbox.com/vector-tiles) that can be viewed with a
vector-tile capable slippy-map viewer. The tiles contain road geometries and metadata that can
be used to examine the routing graph. The tiles are generated directly from the data in-memory,
so are in sync with actual routing results, and let you examine which roads are actually routable,
and what weights they have applied.

**Parameters**

-   `ZXY` **[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)** an array consisting of `x`, `y`, and `z` values representing tile coordinates like
    [wiki.openstreetmap.org/wiki/Slippy_map_tilenames](https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames)
    and are supported by vector tile viewers like [Mapbox GL JS]\(<https://www.mapbox.com/mapbox-gl-js/api/>.
-   `callback` **[Function](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/function)** 

**Examples**

```javascript
var osrm = new OSRM('network.osrm');
osrm.tile([0, 0, 0], function(err, response) {
  if (err) throw err;
  fs.writeFileSync('./tile.vector.pbf', response); // write the buffer to a file
});
```

Returns **[Buffer](https://nodejs.org/api/buffer.html)** contains a Protocol Buffer encoded vector tile.

## match

Map matching matches given GPS points to the road network in the most plausible way.
Please note the request might result multiple sub-traces. Large jumps in the timestamps
(>60s) or improbable transitions lead to trace splits if a complete matching could
not be found. The algorithm might not be able to match all points. Outliers are removed
if they can not be matched successfully.

**Parameters**

-   `options` **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** Object literal containing parameters for the match query.
    -   `options.steps` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)]** Return route steps for each route. (optional, default `false`)
    -   `options.annotations` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)] or \[[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)>]** Return annotations for each route leg for duration, nodes, distance, weight, datasources and/or speed. Annotations can be `false` or `true` (no/full annotations) or an array of strings with `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed`. (optional, default `false`)
    -   `options.geometries` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Returned route geometry format (influences overview
        and per step). Can also be `geojson`. (optional, default `polyline`)
    -   `options.overview` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Add overview geometry either `full`, `simplified`
        according to highest zoom level it could be display on, or not at all (`false`). (optional, default `simplified`)
    -   `options.timestamps` **\[[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;[Number](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number)>]** Timestamp of the input location (integers, UNIX-like timestamp).
    -   `options.radiuses` **\[[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)]** Standard deviation of GPS precision used for map matching.
        If applicable use GPS accuracy (`double >= 0`, default `5m`).
-   `callback` **[Function](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/function)** 

**Examples**

```javascript
var osrm = new OSRM('network.osrm');
var options = {
    coordinates: [[13.393252,52.542648],[13.39478,52.543079],[13.397389,52.542107]],
    timestamps: [1424684612, 1424684616, 1424684620]
};
osrm.match(options, function(err, response) {
    if (err) throw err;
    console.log(response.tracepoints); // array of Waypoint objects
    console.log(response.matchings); // array of Route objects
});
```

Returns **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** containing `tracepoints` and `matchings`.
**`tracepoints`** Array of [`Ẁaypoint`](#waypoint) objects representing all points of the trace in order.
If the trace point was ommited by map matching because it is an outlier, the entry will be null. Each
`Waypoint` object includes two additional properties, 1) `matchings_index`: Index to the
[`Route`](#route) object in matchings the sub-trace was matched to, 2) `waypoint_index`: Index of
the waypoint inside the matched route.
**`matchings`** is an array of [`Route`](#route) objects that
assemble the trace. Each `Route` object has an additional `confidence` property, which is the confidence of
the matching. float value between `0` and `1`. `1` is very confident that the matching is correct.

## trip

The trip plugin solves the Traveling Salesman Problem using a greedy heuristic (farthest-insertion algorithm). The returned path does not have to be the fastest path, as TSP is NP-hard it is only an approximation. Note that all input coordinates have to be connected for the trip service to work.

**Parameters**  
-   `options` **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** Object literal containing parameters for the trip query.
    -   `options.roundtrip` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)]** Return route is a roundtrip. (optional, default `true`)
    -   `options.source` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Return route starts at `any` coordinate. Can also be `first`. (optional, default `any`)
    -   `options.destination` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Return route ends at `any` coordinate. Can also be `last`. (optional, default `any`)
    -   `options.steps` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)]** Return route steps for each route. (optional, default `false`)
    -   `options.annotations` **\[[Boolean](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)] or \[[Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)>]** Return annotations for each route leg for duration, nodes, distance, weight, datasources and/or speed. Annotations can be `false` or `true` (no/full annotations) or an array of strings with `duration`, `nodes`, `distance`, `weight`, `datasources`, `speed`. (optional, default `false`)
    -   `options.geometries` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Returned route geometry format (influences overview
        and per step). Can also be `geojson`. (optional, default `polyline`)
    -   `options.overview` **\[[String](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)]** Add overview geometry either `full`, `simplified` (optional, default `simplified`)
-   `callback` **[Function](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/function)** 

**Fixing Start and End Points**

It is possible to explicitly set the start or end coordinate of the trip. When source is set to `first`, the first coordinate is used as start coordinate of the trip in the output. When destination is set to `last`, the last coordinate will be used as destination of the trip in the returned output. If you specify `any`, any of the coordinates can be used as the first or last coordinate in the output.

However, if `source=any&destination=any` the returned round-trip will still start at the first input coordinate by default.

Currently, not all combinations of `roundtrip`, `source` and `destination` are supported.
Right now, the following combinations are possible:

| roundtrip | source | destination | supported |
| :-- | :-- | :-- | :-- |
| true | first | last | **yes** | 
| true | first | any | **yes** |
| true | any | last | **yes** |
| true | any | any | **yes** |
| false | first | last | **yes** |
| false | first | any | no |
| false | any | last | no |
| false | any | any | no |

**Examples**

Roundtrip Request
```javascript
var osrm = new OSRM('network.osrm');
var options = {
  coordinates: [
    [13.36761474609375, 52.51663871100423],
    [13.374481201171875, 52.506191342034576]
  ]
}
osrm.trip(options, function(err, response) {
  if (err) throw err;
  console.log(response.waypoints); // array of Waypoint objects
  console.log(response.trips); // array of Route objects
});
```

Non Roundtrip Request
```javascript
var osrm = new OSRM('network.osrm');
var options = {
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
  console.log(response.waypoints); // array of Waypoint objects
  console.log(response.trips); // array of Route objects
});
```

Returns **[Object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)** containing `waypoints` and `trips`.
**`waypoints`**: an array of [`Ẁaypoint`](#waypoint) objects representing all waypoints in input order.
Each Waypoint object has the following additional properties, 1) `trips_index`: index to trips of the
sub-trip the point was matched to, and 2) `waypoint_index`: index of the point in the trip.
**`trips`**: an array of [`Route`](#route) objects that assemble the trace.

# Responses

Responses

## Route

Represents a route through (potentially multiple) waypoints.

**Parameters**

-   `exteral` **documentation** in [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#route)

## RouteLeg

Represents a route between two waypoints.

**Parameters**

-   `exteral` **documentation** in [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#routeleg)

## RouteStep

A step consists of a maneuver such as a turn or merge, followed by a distance of travel along a single way to the subsequent step.

**Parameters**

-   `exteral` **documentation** in [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#routestep)

## StepManeuver

**Parameters**

-   `exteral` **documentation** in [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#stepmanuever)

## Waypoint

Object used to describe waypoint on a route.

**Parameters**

-   `exteral` **documentation** in [`osrm-backend`](https://github.com/Project-OSRM/osrm-backend/blob/master/docs/http.md#waypoint)
