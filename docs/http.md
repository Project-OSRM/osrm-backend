## Environent Variables

### SIGNAL_PARENT_WHEN_READY

If the SIGNAL_PARENT_WHEN_READY environment variable is set osrm-routed will
send the USR1 signal to its parent when it will be running and waiting for
requests. This could be used to upgrade osrm-routed to a new binary on the fly
without any service downtime - no incoming requests will be lost.

### DISABLE_ACCESS_LOGGING

If the DISABLE_ACCESS_LOGGING environment variable is set osrm-routed will
**not** log any http requests to standard output. This can be useful in high
traffic setup.

## HTTP API

`osrm-routed` supports only `GET` requests of the form. If you your response size
exceeds the limits of a simple URL encoding, consider using our [NodeJS bindings](https://github.com/Project-OSRM/node-osrm)
or using the [C++ library directly](libosrm.md).

### Request

```
http://{server}/{service}/{version}/{profile}/{coordinates}[.{format}]?option=value&option=value
```

- `server`: location of the server. Example: `127.0.0.1:5000` (default)
- `service`: Name of the service to be used. Support are the following services:
  
    | Service     |           Description                                     |
    |-------------|-----------------------------------------------------------|
    | [`route`](#service-route)     | shortest path between given coordinates                   |
    | [`nearest`](#service-nearest)   | returns the nearest street segment for a given coordinate |
    | [`table`](#service-table)     | computes distance tables for given coordinates            |
    | [`match`](#service-match)     | matches given coordinates to the road network             |
    | [`trip`](#service-trip)      | Compute the shortest round trip between given coordinates |
    | [`tile`](#service-tile)      | Return vector tiles containing debugging info             |
  
- `version`: Version of the protocol implemented by the service.
- `profile`: Mode of transportation, is determined by the profile that is used to prepare the data
- `coordinates`: String of format `{longitude},{latitude};{longitude},{latitude}[;{longitude},{latitude} ...]` or `polyline({polyline})`.
- `format`: Only `json` is supportest at the moment. This parameter is optional and defaults to `json`.

Passing any `option=value` is optional. `polyline` follows Google's polyline format with precision 5 and can be generated using [this package](https://www.npmjs.com/package/polyline).
To pass parameters to each location some options support an array like encoding:

```
{option}={element};{element}[;{element} ... ]
```

The number of elements must match exactly the number of locations. If you don't want to pass a value but instead use the default you can pass an empty `element`.

Example: 2nd location use the default value for `option`:

```
{option}={element};;{element}
```

## General options

| Option     | Values                                                 | Description                                      |
|------------|--------------------------------------------------------|--------------------------------------------------|
|bearings    |`{bearing};{bearing}[;{bearing} ...]`                   |Limits the search to segments with given bearing in degrees towards true north in clockwise direction. |
|radiuses    |`{radius};{radius}[;{radius} ...]`                      |Limits the search to given radius in meters.      |
|hints       |`{hint};{hint}[;{hint} ...]`                            |Hint to derive position in street network.        |

Where the elements follow the following format:

| Element    | Values                                                 |
|------------|--------------------------------------------------------|
|bearing     |`{value},{range}` `integer 0 .. 360,integer 0 .. 180`  |
|radius      |`double >= 0` or `unlimited` (default)                  |
|hint        |Base64 `string`                                         |

#### Examples

Query on Berlin with three coordinates:

```
http://router.project-osrm.org/route/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?overview=false
```

Using polyline:

```
http://router.project-osrm.org/route/v1/driving/polyline(ofp_Ik_vpAilAyu@te@g`E)?overview=false
```

### Response

Every response object has a `code` field.

```json
{
"code": {code},
"message": {message}
}
```

Where `code` is on one of the strings below or service dependent:

| Type              | Description                                                                      |
|-------------------|----------------------------------------------------------------------------------|
| `Ok`              | Request could be processed as expected.                                          |
| `InvalidUrl`      | URL string is invalid.                                                           |
| `InvalidService`  | Service name is invalid.                                                         |
| `InvalidVersion`  | Version is not found.                                                            |
| `InvalidOptions`  | Options are invalid.                                                             |
| `NoSegment`       | One of the supplied input coordinates could not snap to street segment.          |
| `TooBig`          | The request size violates one of the service specific request size restrictions. |

`message` is a **optional** human-readable error message. All other status types are service dependent.

In case of an error the HTTP status code will be `400`. Otherwise the HTTP status code will be `200` and `code` will be `Ok`.

## Service `nearest`

Snaps a coordinate to the street network and returns the nearest n matches.

### Request

```
http://{server}/nearest/v1/{profile}/{coordinates}.json?number={number}
```

Where `coordinates` only supports a single `{longitude},{latitude}` entry.

In addition to the [general options](#general-options) the following options are supported for this service:

|Option      |Values                        |Description                                         |
|------------|------------------------------|----------------------------------------------------|
|number      |`integer >= 1` (default `1`)  |Number of nearest segments that should be returned. |

### Response

- `code` if the request was successful `Ok` otherwise see the service dependent and general status codes.
- `waypoints` array of `Waypoint` objects sorted by distance to the input coordinate. Each object has at least the following additional properties:
  - `distance`: Distance in meters to the supplied input coordinate.

### Examples

Querying nearest three snapped locations of `13.388860,52.517037` with a bearing between `20° - 340°`.

```
http://router.project-osrm.org/nearest/v1/driving/13.388860,52.517037?number=3&bearings=0,20
```

## Service `route`

### Request

```
http://{server}/route/v1/{profile}/{coordinates}?alternatives={true|false}&steps={true|false}&geometries={polyline|geojson}&overview={full|simplified|false}&annotations={true|false}
```

In addition to the [general options](#general-options) the following options are supported for this service:

|Option      |Values                                    |Description                                                                    |
|------------|------------------------------------------|-------------------------------------------------------------------------------|
|alternatives|`true`, `false` (default)                 |Search for alternative routes and return as well.\*                            |
|steps       |`true`, `false` (default)                 |Return route steps for each route leg                                          |
|annotations |`true`, `false` (default)                 |Returns additional metadata for each coordinate along the route geometry.      |
|geometries  |`polyline` (default), `geojson`           |Returned route geometry format (influences overview and per step)             |
|overview    |`simplified` (default), `full`, `false`   |Add overview geometry either full, simplified according to highest zoom level it could be display on, or not at all.|
|continue_straight |`default` (default), `true`, `false`|Forces the route to keep going straight at waypoints and don't do a uturn even if it would be faster. Default value depends on the profile. |

\* Please note that even if an alternative route is requested, a result cannot be guaranteed.

### Response

- `code` if the request was successful `Ok` otherwise see the service dependent and general status codes.
- `waypoints`: Array of `Waypoint` objects representing all waypoints in order:
- `routes`: An array of `Route` objects, ordered by descending recommendation rank.

In case of error the following `code`s are supported in addition to the general ones:

| Type              | Description     |
|-------------------|-----------------|
| `NoRoute`        | No route found. |

All other fields might be undefined.

## Service `table`
### Request
```
http://{server}/table/v1/{profile}/{coordinates}?{sources}=[{elem}...];&destinations=[{elem}...]`
```

This computes duration tables for the given locations. Allows for both symmetric and asymmetric tables.

### Coordinates

In addition to the [general options](#general-options) the following options are supported for this service:

|Option      |Values                                            |Description                                  |
|------------|--------------------------------------------------|---------------------------------------------|
|sources     |`{index};{index}[;{index} ...]` or `all` (default)|Use location with given index as source.     |
|destinations|`{index};{index}[;{index} ...]` or `all` (default)|Use location with given index as destination.|

Unlike other array encoded options, the length of `sources` and `destinations` can be **smaller or equal**
to number of input locations;

Example:

```
sources=0;5;7&destinations=5;1;4;2;3;6
```

|Element     |Values                       |
|------------|-----------------------------|
|index       |`0 <= integer < #locations`  |

### Response

- `code` if the request was successful `Ok` otherwise see the service dependent and general status codes.
- `durations` array of arrays that stores the matrix in row-major order. `durations[i][j]` gives the travel time from
  the i-th waypoint to the j-th waypoint. Values are given in seconds.
- `sources` array of `Waypoint` objects describing all sources in order
- `destinations` array of `Waypoint` objects describing all destinations in order

In case of error the following `code`s are supported in addition to the general ones:

| Type              | Description     |
|-------------------|-----------------|
| `NoTable`        | No route found. |

All other fields might be undefined.

#### Examples

Returns a `3x3` matrix:
```
http://router.project-osrm.org/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219
```

Returns a `1x3` matrix:
```
http://router.project-osrm.org/table/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?sources=0
```

Returns a asymmetric 3x2 matrix with from the polyline encoded locations `qikdcB}~dpXkkHz`:
```
http://router.project-osrm.org/table/v1/driving/qikdcB}~dpXkkHz?sources=0;1;3&destinations=2;4
```

## Service `match`

Map matching matches given GPS points to the road network in the most plausible way.
Please note the request might result multiple sub-traces. Large jumps in the timestamps (>60s) or improbable transitions lead to trace splits if a complete matching could not be found.
The algorithm might not be able to match all points. Outliers are removed if they can not be matched successfully.

### Request

```
http://{server}/match/v1/{profile}/{coordinates}?steps={true|false}&geometries={polyline|geojson}&overview={simplified|full|false}&annotations={true|false}
```

In addition to the [general options](#general-options) the following options are supported for this service:


|Option      |Values                                          |Description                                                                               |
|------------|------------------------------------------------|------------------------------------------------------------------------------------------|
|steps       |`true`, `false` (default)                       |Return route steps for each route                                                         |
|geometries  |`polyline` (default), `geojson`                 |Returned route geometry format (influences overview and per step)                        |
|annotations |`true`, `false` (default)                       |Returns additional metadata for each coordinate along the route geometry.                |
|overview    |`simplified` (default), `full`, `false`         |Add overview geometry either full, simplified according to highest zoom level it could be display on, or not at all.|
|timestamps  |`{timestamp};{timestamp}[;{timestamp} ...]`     |Timestamp of the input location.                                                          |
|radiuses    |`{radius};{radius}[;{radius} ...]`              |Standard deviation of GPS precision used for map matching. If applicable use GPS accuracy.|

|Parameter   |Values                        |
|------------|------------------------------|
|timestamp   |`integer` UNIX-like timestamp |
|radius      |`double >= 0` (default 5m)    |

### Response
- `code` if the request was successful `Ok` otherwise see the service dependent and general status codes.
- `tracepoints`: Array of `Ẁaypoint` objects representing all points of the trace in order.
  If the trace point was ommited by map matching because it is an outlier, the entry will be `null`.
  Each `Waypoint` object has the following additional properties:
  - `matchings_index`: Index to the `Route` object in `matchings` the sub-trace was matched to.
  - `waypoint_index`: Index of the waypoint inside the matched route.
- `matchings`: An array of `Route` objects that assemble the trace. Each `Route` object has the following additional properties:
  - `confidence`: Confidence of the matching. `float` value between 0 and 1. 1 is very confident that the matching is correct.

In case of error the following `code`s are supported in addition to the general ones:

| Type              | Description         |
|-------------------|---------------------|
| `NoMatch`         | No matchings found. |

All other fields might be undefined.

## Service `trip`

The trip plugin solves the Traveling Salesman Problem using a greedy heuristic (farthest-insertion algorithm).
The returned path does not have to be the shortest path, as TSP is NP-hard it is only an approximation.
Note that if the input coordinates can not be joined by a single trip (e.g. the coordinates are on several disconnected islands)
multiple trips for each connected component are returned.

### Request

```
http://{server}/trip/v1/{profile}/{coordinates}?steps={true|false}&geometries={polyline|geojson}&overview={simplified|full|false}&annotations={true|false}
```

In addition to the [general options](#general-options) the following options are supported for this service:

|Option      |Values                                          |Description                                                                |
|------------|------------------------------------------------|---------------------------------------------------------------------------|
|steps       |`true`, `false` (default)                       |Return route instructions for each trip                                    |
|annotations |`true`, `false` (default)                       |Returns additional metadata for each coordinate along the route geometry.      |
|geometries  |`polyline` (default), `geojson`                 |Returned route geometry format (influences overview and per step)         |
|overview    |`simplified` (default), `full`, `false`         |Add overview geometry either full, simplified according to highest zoom level it could be display on, or not at all.|

### Response

- `code` if the request was successful `Ok` otherwise see the service dependent and general status codes.
- `waypoints`: Array of `Waypoint` objects representing all waypoints in input order. Each `Waypoint` object has the following additional properties:
  - `trips_index`: Index to `trips` of the sub-trip the point was matched to.
  - `waypoint_index`: Index of the point in the trip.
- `trips`: An array of `Route` objects that assemble the trace.

In case of error the following `code`s are supported in addition to the general ones:

| Type              | Description         |
|-------------------|---------------------|
| `NoTrips`        | No trips found.     |

All other fields might be undefined.

## Result objects

### Route

Represents a route through (potentially multiple) waypoints.

#### Properties

- `distance`: The distance traveled by the route, in `float` meters.
- `duration`: The estimated travel time, in `float` number of seconds.
- `geometry`: The whole geometry of the route value depending on `overview` parameter, format depending on the `geometries` parameter. See `RouteStep`'s `geometry` field for a parameter documentation.
  
  | overview   | Description                 |
  |------------|-----------------------------|
  | simplified | Geometry is simplified according to the highest zoom level it can still be displayed on full. |
  | full       | Geometry is not simplified. |
  | false      | Geometry is not added.      |
  
- `legs`: The legs between the given waypoints, an array of `RouteLeg` objects.

#### Example

Three input coordinates, `geometry=geojson`, `steps=false`:

```json
{
  "distance": 90.0,
  "duration": 300.0,
  "geometry": {"type": "LineString", "coordinates": [[120.0, 10.0], [120.1, 10.0], [120.2, 10.0], [120.3, 10.0]]},
  "legs": [
    {
      "distance": 30.0,
      "duration": 100.0,
      "steps": []
    },
    {
      "distance": 60.0,
      "duration": 200.0,
      "steps": []
    }
  ]
}
```

### RouteLeg

Represents a route between two waypoints.

#### Properties

- `distance`: The distance traveled by this route leg, in `float` meters.
- `duration`: The estimated travel time, in `float` number of seconds.
- `summary`: Summary of the route taken as `string`. Depends on the `steps` parameter:
   
   | steps        |                                                                       |
   |--------------|-----------------------------------------------------------------------|
   | true         | Names of the two major roads used. Can be empty if route is too short.|
   | false        | empty `string`                                                        |

- `steps`: Depends on the `steps` parameter.
   
   | steps        |                                                                       |
   |--------------|-----------------------------------------------------------------------|
   | true         | array of `RouteStep` objects describing the turn-by-turn instructions |
   | false        | empty array                                                           |

- `annotation`: Additional details about each coordinate along the route geometry:

   | annotations  |                                                                       |
   |--------------|-----------------------------------------------------------------------|
   | true         | returns distance and durations of each coordinate along the route     |
   | false        | will not exist                                                        |

#### Example

With `steps=false` and `annotations=true`:

```json
{
  "distance": 30.0,
  "duration": 100.0,
  "steps": [],
  "annotation": {
    "distance": [5,5,10,5,5],
    "duration": [15,15,40,15,15],
    "datasources": [1,0,0,0,1],
    "nodes": [49772551,49772552,49786799,49786800,49786801,49786802]
  }
}
```

#### Annotation data

Several fields are available as annotations.  They are:

   | field       | description                                                                                             |
   |-------------|---------------------------------------------------------------------------------------------------------|
   | distance    | the distance, in metres, between each pair of coordinates                                               |
   | duration    | the duration between each pair of coordinates, in seconds                                               |
   | datasources | the index of the datasource for the speed between each pair of coordinates. `0` is the default profile, other values are supplied via `--segment-speed-file` to `osrm-contract`                                 |
   | nodes       | the OSM node ID for each coordinate along the route, excluding the first/last user-supplied coordinates |

### RouteStep

A step consists of a maneuver such as a turn or merge, followed
by a distance of travel along a single way to the subsequent
step.

#### Properties

- `distance`: The distance of travel from the maneuver to the subsequent step, in `float` meters.
- `duration`: The estimated travel time, in `float` number of seconds.
- `geometry`: The unsimplified geometry of the route segment, depending on the `geometries` parameter.
  
  | geometries |                                                                    |
  |------------|--------------------------------------------------------------------|
  | polyline   | [polyline](https://www.npmjs.com/package/polyline) with precision 5 in [latitude,longitude] encoding |
  | geojson    | [GeoJSON `LineString`](http://geojson.org/geojson-spec.html#linestring) or [GeoJSON `Point`](http://geojson.org/geojson-spec.html#point) if it is only one coordinate (not wrapped by a GeoJSON feature)|
  
- `name`: The name of the way along which travel proceeds.
- `pronunciation`: The pronunciation hint of the way name. Will be `undefined` if there is no pronunciation hit.
- `destinations`: The destinations of the way. Will be `undefined` if there are no destinations.
- `mode`: A string signifying the mode of transportation.
- `maneuver`: A `StepManeuver` object representing the maneuver.
- `intersections`: A list of `Intersection` objects that are passed along the segment, the very first belonging to the StepManeuver

#### Example

```
{
 "distance":152.3,
 "duration":15.6,
 "name":"Lortzingstraße",
 "maneuver":{
     "type":"turn",
     "modifier":"right",
     },
 "geometry":"{lu_IypwpAVrAvAdI",
 "mode":"driving",
 "intersections":[
    {"location":[13.39677,52.54366],
    "in":3,
    "out":2,
    "bearings":[10,92,184,270],
    "entry":["true","true","true","false"],
    "lanes":[
         {"indications":["left","straight"], "valid":"false"},
         {"indications":["right"], "valid":"true"}
    ]},
    {"location":[13.394718,52.543096],
    "in":0,
    "out":1,
    "bearings":[60,240,330],
    "entry":["false","true","true"]
    "lanes":[
         {"indications":["straight"], "valid":"true"},
         {"indications":["right"], "valid":"false"}
     ]}
]}
```

### StepManeuver

#### Properties

- `location`: A `[longitude, latitude]` pair describing the location of the turn.
- `bearing_before`: The clockwise angle from true north to the
  direction of travel immediately before the maneuver.
- `bearing_after`: The clockwise angle from true north to the
  direction of travel immediately after the maneuver.
- `type` A string indicating the type of maneuver. **new identifiers might be introduced without API change**
   Types  unknown to the client should be handled like the `turn` type, the existance of correct `modifier` values is guranteed.
  
  | `type`           | Description                                                  |
  |------------------|--------------------------------------------------------------|
  | `turn`           | a basic turn into direction of the `modifier`                |
  | `new name`       | no turn is taken/possible, but the road name changes. The road can take a turn itself, following `modifier`.                  |
  | `depart`         | indicates the departure of the leg                           |
  | `arrive`         | indicates the destination of the leg                         |
  | `merge`          | merge onto a street (e.g. getting on the highway from a ramp, the `modifier specifies the direction of the merge`) |
  | `ramp`           | **Deprecated**. Replaced by `on_ramp` and `off_ramp`.        |
  | `on ramp`        | take a ramp to enter a highway (direction given my `modifier`) |
  | `off ramp`       | take a ramp to exit a highway (direction given my `modifier`)  |
  | `fork`           | take the left/right side at a fork depending on `modifier`   |
  | `end of road`    | road ends in a T intersection turn in direction of `modifier`|
  | `use lane`       | going straight on a specific lane                            |
  | `continue`       | Turn in direction of `modifier` to stay on the same road     |
  | `roundabout`     | traverse roundabout, has additional field `exit` with NR if the roundabout is left. `the modifier specifies the direction of entering the roundabout` |
  | `rotary`         | a larger version of a roundabout, can offer `rotary_name` in addition to the `exit` parameter.  |
  | `roundabout turn`| Describes a turn at a small roundabout that should be treated as normal turn. The `modifier` indicates the turn direciton. Example instruction: `At the roundabout turn left`. |
  | `notification`   | not an actual turn but a change in the driving conditions. For example the travel mode.  If the road takes a turn itself, the `modifier` describes the direction |

  Please note that even though there are `new name` and `notification` instructions, the `mode` and `name` can change
  between all instructions. They only offer a fallback in case nothing else is to report.

- `modifier` An optional `string` indicating the direction change of the maneuver.
  
  | `modifier`        | Description                               |
  |-------------------|-------------------------------------------|
  | `uturn`           | indicates  reversal of direction          |
  | `sharp right`     | a sharp right turn                        |
  | `right`           | a normal turn to the right                |
  | `slight right`    | a slight turn to the right                |
  | `straight`        | no relevant change in direction           |
  | `slight left`     | a slight turn to the left                 |
  | `left`            | a normal turn to the left                 |
  | `sharp left`      | a sharp turn to the left                  |
  
  The list of turns without a modifier is limited to: `depart/arrive`. If the source/target location is close enough to the `depart/arrive` location, no modifier will be given.
  
  The meaning depends on the `type` field.
  
  | `type`                 | Description                                                                                                               |
  |------------------------|---------------------------------------------------------------------------------------------------------------------------|
  | `turn`                 | `modifier` indicates the change in direction accomplished through the turn                                                |
  | `depart`/`arrive`      | `modifier` indicates the position of departure point and arrival point in relation to the current direction of travel      |
  
- `exit` An optional `integer` indicating number of the exit to take. The field exists for the following `type` field:
  
  | `type`                 | Description                                                                                                               |
  |------------------------|---------------------------------------------------------------------------------------------------------------------------|
  | `roundabout`           | Number of the roundabout exit to take. If exit is `undefined` the destination is on the roundabout.                       |
  | else                   | Indicates the number of intersections passed until the turn. Example instruction: `at the fourth intersection, turn left` |


New properties (potentially depending on `type`) may be introduced in the future without an API version change.

### Lane

A `Lane` represents a turn lane at the corresponding turn location.

#### Properties

- `indications`: a indication (e.g. marking on the road) specifying the turn lane. A road can have multiple indications (e.g. an arrow pointing straight and left). The indications are given in an array, each containing one of the following types. Further indications might be added on without an API version change.
  
  | `value`                | Description                                                                                                               |
  |------------------------|---------------------------------------------------------------------------------------------------------------------------|
  | `none`                 | No dedicated indication is shown.                                                                                         |
  | `uturn`                | An indication signaling the possibility to reverse (i.e. fully bend arrow).                                               |
  | `sharp right`          | An indication indicating a sharp right turn (i.e. strongly bend arrow).                                                   |
  | `right`                | An indication indicating a right turn (i.e. bend arrow).                                                                  |
  | `slight right`         | An indication indicating a slight right turn (i.e. slightly bend arrow).                                                  |
  | `straight`             | No dedicated indication is shown (i.e. straight arrow).                                                                   |
  | `slight left`          | An indication indicating a slight left turn (i.e. slightly bend arrow).                                                   |
  | `left`                 | An indication indicating a left turn (i.e. bend arrow).                                                                   |
  | `sharp left`           | An indication indicating a sharp left turn (i.e. strongly bend arrow).                                                    |
  
- `valid`: a boolean flag indicating whether the lane is a valid choice in the current maneuver

#### Example

```json
{
    "indications": ["left", "straight"],
    "valid": "false"
}
 ```

### Intersection

An intersection gives a full representation of any cross-way the path passes bay. For every step, the very first intersection (`intersections[0]`) corresponds to the
location of the StepManeuver. Further intersections are listed for every cross-way until the next turn instruction.

#### Properties

- `location`: A `[longitude, latitude]` pair describing the location of the turn.
- `bearings`: A list of bearing values (e.g. [0,90,180,270]) that are available at the intersection. The bearings describe all available roads at the intersection.
- `entry`: A list of entry flags, corresponding in a 1:1 relationship to the bearings. A value of `true` indicates that the respective road could be entered on a valid route.
  `false` indicates that the turn onto the respective road would violate a restriction.
- `in`: index into bearings/entry array. Used to calculate the bearing just before the turn. Namely, the clockwise angle from true north to the
  direction of travel immediately before the maneuver/passing the intersection. Bearings are given relative to the intersection. To get the bearing
  in the direction of driving, the bearing has to be rotated by a value of 180. The value is not supplied for `depart` maneuvers.
- `out`: index into the bearings/entry array. Used to extract the bearing just after the turn. Namely, The clockwise angle from true north to the
  direction of travel immediately after the maneuver/passing the intersection. The value is not supplied for `arrive` maneuvers.
- `lanes`: Array of `Lane` objects that denote the available turn lanes at the intersection. If no lane information is available for an intersection, the `lanes` property will not be present.

#### Example
```
{
    "location":[13.394718,52.543096],
    "in":0,
    "out":2,
    "bearings":[60,150,240,330],
    "entry":["false","true","true","true"]
    "lanes":{
        "indications": ["left", "straight"],
        "valid": "false"
    }
                                                                                                                               ]}
}
```

### Waypoint

Object used to describe waypoint on a route.

#### Properties

- `name` Name of the street the coordinate snapped to
- `location` Array that contains the `[longitude, latitude]` pair of the snapped coordinate
- `hint` Unique internal identifier of the segment (ephemeral, not constant over data updates)
   This can be used on subsequent request to significantly speed up the query and to connect multiple services.
   E.g. you can use the `hint` value obtained by the `nearest` query as `hint` values for `route` inputs.

## Service `tile`

This generates [Mapbox Vector Tiles](https://www.mapbox.com/developers/vector-tiles/) that can be viewed with a vector-tile capable slippy-map viewer.  The tiles contain road geometries and metadata that can be used to examine the routing graph.  The tiles are generated directly from the data in-memory, so are in sync with actual routing results, and let you examine which roads are actually routable, and what weights they have applied.

### Request
```
http://{server}/tile/v1/{profile}/tile({x},{y},{zoom}).mvt
```

The `x`, `y`, and `zoom` values are the same as described at https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames, and are supported by vector tile viewers like [Mapbox GL JS](https://www.mapbox.com/mapbox-gl-js/api/).

### Response

The response object is either a binary encoded blob with a `Content-Type` of `application/x-protobuf`, or a `404` error.  Note that OSRM is hard-coded to only return tiles from zoom level 12 and higher (to avoid accidentally returning extremely large vector tiles).

Vector tiles contain just a single layer named `speeds`.  Within that layer, features can have `speed` (int) and `is_small` (boolean) attributes.
