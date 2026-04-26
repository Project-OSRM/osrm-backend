# Python API

The Python bindings provide access to OSRM's routing services through the `osrm` package. Install with `pip install osrm-bindings`.

## OSRM

The `OSRM` class is the main entry point. It requires a `.osrm.*` dataset prepared by the OSRM toolchain.

```python
import osrm

# From file
engine = osrm.OSRM("path/to/data.osrm")

# With keyword arguments
engine = osrm.OSRM(
    storage_config="path/to/data.osrm",
    algorithm="CH",                      # or "MLD"
    use_shared_memory=False,
    max_locations_trip=3,
    max_locations_viaroute=3,
    max_locations_distance_table=3,
    max_locations_map_matching=3,
    max_results_nearest=1,
    max_alternatives=1,
    default_radius="unlimited",
)

# Using shared memory (requires osrm-datastore)
engine = osrm.OSRM(use_shared_memory=True)
```

### Parameters

- **`storage_config`** `str` - Path to the `.osrm` dataset.
- **`algorithm`** `str` - Routing algorithm: `"CH"` or `"MLD"`. Default: `"CH"`.
- **`use_shared_memory`** `bool` - Connect to shared memory datastore. Default: `True`.
- **`dataset_name`** `str` - Named shared memory dataset (requires `osrm-datastore --dataset_name`).
- **`memory_file`** `str` - **Deprecated.** Equivalent to `use_mmap=True`.
- **`use_mmap`** `bool` - Memory-map files instead of loading into RAM.
- **`max_locations_trip`** `int` - Max locations in trip queries.
- **`max_locations_viaroute`** `int` - Max locations in route queries.
- **`max_locations_distance_table`** `int` - Max locations in table queries.
- **`max_locations_map_matching`** `int` - Max locations in match queries.
- **`max_results_nearest`** `int` - Max results in nearest queries.
- **`max_alternatives`** `int` - Max alternative routes.
- **`default_radius`** `float | "unlimited"` - Default search radius in meters.

### Services

All service methods take a parameters object and return a dict-like `Object`:

```python
result = engine.Route(route_params)
print(result["routes"])
print(result["waypoints"])
```

## Route

Finds the fastest route between two or more coordinates.

```python
params = osrm.RouteParameters(
    coordinates=[(7.41337, 43.72956), (7.41546, 43.73077)],
    steps=True,
    alternatives=2,
    annotations=["speed", "duration"],
    geometries="geojson",
    overview="full",
)
result = engine.Route(params)
```

### RouteParameters

Inherits all [BaseParameters](#baseparameters).

- **`steps`** `bool` - Return route steps for each leg. Default: `False`.
- **`alternatives`** `int` - Number of alternative routes to search for. Default: `0`.
- **`annotations`** `list[str]` - Additional metadata: `"none"`, `"duration"`, `"nodes"`, `"distance"`, `"weight"`, `"datasources"`, `"speed"`, `"all"`. Default: `[]`.
- **`geometries`** `str` - Geometry format: `"polyline"`, `"polyline6"`, `"geojson"`. Default: `"polyline"`.
- **`overview`** `str` - Overview geometry: `"simplified"`, `"full"`, `"false"`. Default: `"simplified"`.
- **`continue_straight`** `bool | None` - Force route to continue straight at waypoints.
- **`waypoints`** `list[int]` - Indices of coordinates to treat as waypoints. Must include first and last.

## Table

Computes duration/distance matrices between coordinates.

```python
params = osrm.TableParameters(
    coordinates=[(7.41337, 43.72956), (7.41546, 43.73077), (7.41862, 43.73216)],
    sources=[0],
    destinations=[1, 2],
    annotations=["duration", "distance"],
)
result = engine.Table(params)
```

### TableParameters

Inherits all [BaseParameters](#baseparameters).

- **`sources`** `list[int]` - Indices of source coordinates. Default: all.
- **`destinations`** `list[int]` - Indices of destination coordinates. Default: all.
- **`annotations`** `list[str]` - `"duration"`, `"distance"`, `"all"`. Default: `["duration"]`.
- **`fallback_speed`** `float` - Speed for crow-flies fallback when no route found.
- **`fallback_coordinate_type`** `str` - `"input"` or `"snapped"`.
- **`scale_factor`** `float` - Scales duration values. Default: `1.0`.

## Nearest

Finds the nearest street segment for a coordinate.

```python
params = osrm.NearestParameters(
    coordinates=[(7.41337, 43.72956)],
    number_of_results=3,
)
result = engine.Nearest(params)
```

### NearestParameters

Inherits all [BaseParameters](#baseparameters).

- **`number_of_results`** `int` - Number of nearest segments to return. Default: `1`.

## Match

Snaps noisy GPS traces to the road network.

```python
params = osrm.MatchParameters(
    coordinates=[(7.41337, 43.72956), (7.41546, 43.73077), (7.41862, 43.73216)],
    timestamps=[1424684612, 1424684616, 1424684620],
    radiuses=[5.0, 5.0, 5.0],
    annotations=["speed"],
    geometries="geojson",
)
result = engine.Match(params)
```

### MatchParameters

Inherits all [RouteParameters](#routeparameters) and [BaseParameters](#baseparameters).

- **`timestamps`** `list[int]` - UNIX timestamps for each coordinate.
- **`gaps`** `str` - Gap handling: `"split"` or `"ignore"`. Default: `"split"`.
- **`tidy`** `bool` - Remove duplicates. Default: `False`.
- **`waypoints`** `list[int]` - Indices of coordinates to treat as waypoints.

## Trip

Solves the Traveling Salesman Problem for the given coordinates.

```python
params = osrm.TripParameters(
    coordinates=[(7.41337, 43.72956), (7.41546, 43.73077), (7.41862, 43.73216)],
    source="first",
    destination="last",
    roundtrip=True,
    annotations=["duration"],
    geometries="geojson",
)
result = engine.Trip(params)
```

### TripParameters

Inherits all [RouteParameters](#routeparameters) and [BaseParameters](#baseparameters).

- **`source`** `str` - `"any"` or `"first"`. Default: `"any"`.
- **`destination`** `str` - `"any"` or `"last"`. Default: `"any"`.
- **`roundtrip`** `bool` - Return to first location. Default: `True`.

## Tile

Generates vector tiles with internal routing graph data.

```python
params = osrm.TileParameters(x=17059, y=11948, z=15)
result = engine.Tile(params)  # returns bytes
```

### TileParameters

- **`x`** `int` - Tile x coordinate.
- **`y`** `int` - Tile y coordinate.
- **`z`** `int` - Tile zoom level.

## BaseParameters

Shared parameters inherited by Nearest, Table, Route, Match, and Trip.

- **`coordinates`** `list[tuple[float, float]]` - List of `(longitude, latitude)` pairs.
- **`hints`** `list[str | None]` - Base64-encoded hints from previous requests.
- **`radiuses`** `list[float | None]` - Search radius per coordinate in meters. `None` for unlimited.
- **`bearings`** `list[tuple[int, int] | None]` - `(bearing, range)` pairs in degrees. `None` for unrestricted.
- **`approaches`** `list[str | None]` - `"curb"`, `"unrestricted"`, or `None`.
- **`generate_hints`** `bool` - Include hints in response. Default: `True`.
- **`exclude`** `list[str]` - Road classes to avoid (e.g. `["motorway"]`).
- **`snapping`** `str` - `"default"` or `"any"`. Default: `"default"`.

## Types

### Coordinate

```python
coord = osrm.Coordinate((7.41337, 43.72956))
print(coord.lon, coord.lat)
```

### Bearing

```python
bearing = osrm.Bearing((200, 180))
print(bearing.bearing, bearing.range)
```

### Object / Array

Service results are returned as `Object` (dict-like) and `Array` (list-like) wrappers around OSRM's internal JSON types. They support `[]`, `len()`, `in`, and iteration.

```python
result = engine.Route(params)
for route in result["routes"]:
    print(route["distance"], route["duration"])
```

## CLI

The package also installs OSRM command-line tools, accessible via `python -m osrm`:

```bash
python -m osrm extract data.osm.pbf -p profiles/car.lua
python -m osrm contract data.osrm
python -m osrm partition data.osrm
python -m osrm customize data.osrm
python -m osrm datastore data.osrm
python -m osrm routed data.osrm
```
