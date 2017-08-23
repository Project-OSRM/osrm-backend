# Developing / Debugging guidance code

When changing guidance code, it is easy to introduce problems somewhere in the network.
To get a better feeling of how your changes impact the OSRM experience, we offer ways of generating geojson output to inspect (e.g. with Mapbox Studio).
When you do changes, make sure to inspect a few areas for the impact of the changes.

## How to use GeoJson-Debugging

This is a short guide to describe usage of our GeoJson debug logging mechanism. It is synchronized to guarantee thread-safe logging.

## Outputting into a single file
To use it, the inclusion of `geojson_debug_logger.hpp`    `geojson_debug_policies.hpp` from the `util` directory is required.

Geojson debugging requires a few simple steps to output data into a feature collection.

- Create a Scoped Guard that lives through the process and provide it with all required datastructures (it needs to span the lifetime of all your logging efforts)
- At the location of the output, simply call Write with your own parameters.

A guard (ScopedGeojsonLoggerGuard) requires a logging policy. Per default we provide a way of printing out node-ids as coordinates.

The initialisation to do so looks like this:
`util::ScopedGeojsonLoggerGuard<util::NodeIdVectorToLineString> geojson_guard( "debug.geojson", data-for-conversion);`
Make sure to give the guard a name, so it actually gets a lifetime.

The field `data-for-conversion` can be an arbitrary long set of features and needs to match the parameters used for constructing our policy (in this case `util::NodeIdVectorToLineString`).

The policy itself offers a `operator()` accepting a `vector` of `NodeID`.

For outputting data into our file (debug.geojson), we simply need to call the matching logging routine of the guard: `util::ScioedGeojsonLoggerGuard<util::NodeIdVectorToLineString>::Write(list_of_node_ids);`
(or `guard.Write(list_of_node_ids)` if you created an instance).

### Possible Scopeguard Location
Think of the scopeguard as you would do of any reference. If you wan't to access to logging during a call, the guard object must be alive and valid.

As an example: a good location to create the a scopeguard to log decisions in the edge-based-graph-factory would be right before we run it ([here](https://github.com/Project-OSRM/osrm-backend/blob/a933b5d94943bf3edaf42c84a614a99650d23cba/src/extractor/extractor.cpp#L497)). If you put `util::ScopedGeojsonLoggerGuard<util::NodeIdVectorToLineString> geojson_guard( "debug.geojson", node_coordinate_vector);` at that location, you can then print `util::ScopedGeojsonLoggerGuard<util::NodeIdVectorToLineString>::Write(list_of_node_ids);` anywhere within the `edge-based-graph-factory`.

This location would enable call for all guidance related pre-processing which is called in the edge-based-graph-factory.
Logging any turn-handler decisions, for example, would now be possible.

## Limitations
GeoJson debugging requires a single GeoJsonGuard (ScopedGeojsonLoggerGuard) for each desired output file.
For each set of template parameters, only the most recent guard will actually produce output.

`util::ScopedGeojsonLoggerGuard<util::NodeIdVectorToLineString> geojson_guard( "debug.geojson", data-for-conversion);`

`util::ScopedGeojsonLoggerGuard<util::NodeIdVectorToLineString> geojson_guard( "debug-2.geojson", data-for-conversion);`

Will not provide a way to write into two files, but only `debug-2` will actually contain features.

We cannot nest-these calls.

If we want to use the same policy for multiple files, we need to use different template parameters both for the logger and the guard.

`util::ScopedGeojsonLoggerGuard<util::NodeIdVectorToLineString,0> geojson_guard( "debug.geojson", data-for-conversion);`

`util::ScopedGeojsonLoggerGuard<util::NodeIdVectorToLineString,1> geojson_guard( "debug-2.geojson", data-for-conversion);`

as well as,
 
`util::ScopedGeojsonLoggerGuardr<util::NodeIdVectorToLineString,0>::Write(list_of_node_ids);`

`util::ScopedGeojsonLoggerGuardr<util::NodeIdVectorToLineString,1>::Write(list_of_node_ids);`
