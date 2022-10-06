# cheap-ruler-cpp

Port to C++ of [Cheap Ruler](https://github.com/mapbox/cheap-ruler), a collection of very fast approximations to common geodesic measurements.

[![Build Status](https://travis-ci.org/mapbox/cheap-ruler-cpp.svg?branch=master)](https://travis-ci.org/mapbox/cheap-ruler-cpp)

# Usage

```cpp
#include <mapbox/cheap_ruler.hpp>

namespace cr = mapbox::cheap_ruler;
```

All `point`, `line_string`, `polygon`, and `box` references are [mapbox::geometry](https://github.com/mapbox/geometry.hpp) data structures.

## Create a ruler object

#### `CheapRuler(double latitude, Unit unit)`

Creates a ruler object that will approximate measurements around the given latitude with an optional distance unit. Once created, the ruler object has access to the [methods](#methods) below.

```cpp
auto ruler = cr::CheapRuler(32.8351);
auto milesRuler = cr::CheapRuler(32.8351, cr::CheapRuler::Miles);
```

Possible units:

* `cheap_ruler::CheapRuler::Unit`
* `cheap_ruler::CheapRuler::Kilometers`
* `cheap_ruler::CheapRuler::Miles`
* `cheap_ruler::CheapRuler::NauticalMiles`
* `cheap_ruler::CheapRuler::Meters`
* `cheap_ruler::CheapRuler::Yards`
* `cheap_ruler::CheapRuler::Feet`
* `cheap_ruler::CheapRuler::Inches`

#### `CheapRuler::fromTile(uint32_t y, uint32_t z)`

Creates a ruler object from tile coordinates (`y` and `z` integers). Convenient in tile-reduce scripts.

```cpp
auto ruler = cr::CheapRuler::fromTile(11041, 15);
```

## Methods

#### `distance(point a, point b)`

Given two points of the form [x = longitude, y = latitude], returns the distance (`double`).

```cpp
cr::point point_a{-96.9148, 32.8351};
cr::point point_b{-96.9146, 32.8386};
auto distance = ruler.distance(point_a, point_b);
std::clog << distance; // 0.388595
```

#### `bearing(point a, point b)`

Returns the bearing (`double`) between two points in angles.

```cpp
cr::point point_a{-96.9148, 32.8351};
cr::point point_b{-96.9146, 32.8386};
auto bearing = ruler.bearing(point_a, point_b);
std::clog << bearing; // 2.76206
```

#### `destination(point origin, double distance, double bearing)`

Returns a new point (`point`) given distance and bearing from the starting point.

```cpp
cr::point point_a{-96.9148, 32.8351};
auto dest = ruler.destination(point_a, 1.0, -175);
std::clog << dest.x << ", " << dest.y; // -96.9148, 32.8261
```

#### `offset(point origin, double dx, double dy)`

Returns a new point (`point`) given easting and northing offsets from the starting point.

```cpp
cr::point point_a{-96.9148, 32.8351};
auto os = ruler.offset(point_a, 10.0, -5.0);
std::clog << os.x << ", " << os.y; // -96.808, 32.79
```

#### `lineDistance(const line_string& points)`

Given a line (an array of points), returns the total line distance (`double`).

```cpp
cr::line_string line_a{{ -96.9, 32.8 }, { -96.8, 32.8 }, { -96.2, 32.3 }};
auto line_distance = ruler.lineDistance(line_a);
std::clog << line_distance; // 88.2962
```

#### `area(polygon poly)`

Given a polygon (an array of rings, where each ring is an array of points), returns the area (`double`).

```cpp
cr::linear_ring ring{{ -96.9, 32.8 }, { -96.8, 32.8 }, { -96.2, 32.3 }, { -96.9, 32.8 }};
auto area = ruler.area(cr::polygon{ ring });
std::clog << area; //
```

#### `along(const line_string& line, double distance)`

Returns the point (`point`) at a specified distance along the line.

```cpp
cr::linear_ring ring{{ -96.9, 32.8 }, { -96.8, 32.8 }, { -96.2, 32.3 }, { -96.9, 32.8 }};
auto area = ruler.area(cr::polygon{ ring });
std::clog << area; // 259.581
```

#### `pointOnLine(const line_string& line, point p)`

Returns a tuple of the form `std::pair<point, unsigned>` where point is closest point on the line from the given point, index is the start index of the segment with the closest point, and t is a parameter from 0 to 1 that indicates where the closest point is on that segment.

```cpp
cr::line_string line{{ -96.9, 32.8 }, { -96.8, 32.8 }, { -96.2, 32.3 }};
cr::point point{-96.9, 32.79};
auto pol = ruler.pointOnLine(line, point);
auto point = std::get<0>(pol);
std::clog << point.x << ", " << point.y; // -96.9, 32.8 (point)
std::clog << std::get<1>(pol); // 0 (index)
std::clog << std::get<2>(pol); // 0. (t)
```

#### `lineSlice(point start, point stop, const line_string& line)`

Returns a part of the given line (`line_string`) between the start and the stop points (or their closest points on the line).

```cpp
cr::line_string line{{ -96.9, 32.8 }, { -96.8, 32.8 }, { -96.2, 32.3 }};
cr::point start_point{-96.9, 32.8};
cr::point stop_point{-96.8, 32.8};
auto slice = ruler.lineSlice(start_point, stop_point, line);
std::clog << slice[0].x << ", " << slice[0].y; // -96.9, 32.8
std::clog << slice[1].x << ", " << slice[1].y; // -96.8, 32.8
```

#### `lineSliceAlong(double start, double stop, const line_string& line)`

Returns a part of the given line (`line_string`) between the start and the stop points indicated by distance along the line.

```cpp
cr::line_string line{{ -96.9, 32.8 }, { -96.8, 32.8 }, { -96.2, 32.3 }};
auto slice = ruler.lineSliceAlong(0.1, 1.2, line);
```

#### `bufferPoint(point p, double buffer)`

Given a point, returns a bounding box object ([w, s, e, n]) created from the given point buffered by a given distance.

```cpp
cr::point point{-96.9, 32.8};
auto box = ruler.bufferPoint(point, 0.1);
```

#### `bufferBBox(box bbox, double buffer)`

Given a bounding box, returns the box buffered by a given distance.

```cpp
cr::box bbox({ 30, 38 }, { 40, 39 });
auto bbox2 = ruler.bufferBBox(bbox, 1);
```

#### `insideBBox(point p, box bbox)`

Returns true (`bool`) if the given point is inside in the given bounding box, otherwise false.

```cpp
cr::box bbox({ 30, 38 }, { 40, 39 });
auto inside = ruler.insideBBox({ 35, 38.5 }, bbox);
std::clog << inside; // true
```

# Develop

```shell
# create targets
cmake .

# build
make

# test
./cheap_ruler

# or just do it all in one!
cmake . && make && ./cheap_ruler
```
