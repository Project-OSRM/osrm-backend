## Introduction

OSRM can be used as a library (libosrm) via C++ instead of using it through the HTTP interface and `osrm-routed`. This allows for fine-tuning OSRM and has much less overhead. Here is a quick introduction into how to use `libosrm` in the upcoming v5 release.

Take a look at the example code that lives in the [example directory](https://github.com/Project-OSRM/osrm-backend/tree/master/example). Here is all you ever wanted to know about `libosrm`, that is a short description of what the types do and where to find documentation on it:

## Important interface objects

- [`EngineConfig`](https://github.com/Project-OSRM/osrm-backend/blob/master/include/engine/engine_config.hpp) - for initializing an OSRM instance we can configure certain properties and constraints. E.g. the storage config is the base path such as `france.osm.osrm` from which we derive and load `france.osm.osrm.*` auxiliary files. This also lets you set constraints such as the maximum number of locations allowed for specific services.

- [`OSRM`](https://github.com/Project-OSRM/osrm-backend/blob/master/include/osrm/osrm.hpp) - this is the main Routing Machine type with functions such as `Route` and `Table`. You initialize it with a `EngineConfig`. It does all the heavy lifting for you. Each function takes its own parameters, e.g. the `Route` function takes `RouteParameters`, and a out-reference to a JSON result that gets filled. The return value is a `Status`, indicating error or success.

- [`Status`](https://github.com/Project-OSRM/osrm-backend/blob/master/include/engine/status.hpp) - this is a type wrapping `Error` or `Ok` for indicating error or success, respectively.

- [`TableParameters`](https://github.com/Project-OSRM/osrm-backend/blob/master/include/engine/api/table_parameters.hpp) - this is an example of parameter types the Routing Machine functions expect. In this case `Table` expects its own parameters as `TableParameters`. You can see it wrapping two vectors, sources and destinations --- these are indices into your coordinates for the table service to construct a matrix from (empty sources or destinations means: use all of them). If you ask yourself where coordinates come from, you can see `TableParameters` inheriting from `BaseParameters`.

- [`BaseParameter`](https://github.com/Project-OSRM/osrm-backend/blob/master/include/engine/api/base_parameters.hpp) - this most importantly holds coordinates (and a few other optional properties that you don't need for basic usage); the specific parameter types inherit from `BaseParameters` to get these member attributes. That means your `TableParameters` type has `coordinates`, `sources` and `destination` member attributes (and a few other that we ignore for now).

- [`Coordinate`](https://github.com/Project-OSRM/osrm-backend/blob/master/include/util/coordinate.hpp) - this is a wrapper around a (longitude, latitude) pair. We really don't care about (lon,lat) vs (lat, lon) but we don't want you to accidentally mix them up, so both latitude and longitude are strictly typed wrappers around integers (fixed notation such as `13423240`) and floating points (floating notation such as `13.42324`).

- [Parameters for other services](https://github.com/Project-OSRM/osrm-backend/tree/master/include/engine/api) - here are all other `*Parameters` you need for other Routing Machine services.

- [JSON](https://github.com/Project-OSRM/osrm-backend/blob/master/include/util/json_container.hpp) - this is a sum type resembling JSON. The Routing Machine service functions take a out-ref to a JSON result and fill it accordingly. It is currently implemented using [mapbox/variant](https://github.com/mapbox/variant) which is similar to [Boost.Variant](http://www.boost.org/doc/libs/1_55_0/doc/html/variant.html). There are two ways to work with this sum type: either provide a visitor that acts on each type on visitation or use the `get` function in case you're sure about the structure. The JSON structure is written down in the [HTTP API](#http-api).

## Example

See [the example folder](https://github.com/Project-OSRM/osrm-backend/tree/master/example) in the OSRM repository.

## Workflow

 - Create an `OSRM` instance initialized with a `EngineConfig`
 - Call the service function on the `OSRM` object providing service specific `*Parameters`
 - Check the return code and use the JSON result
