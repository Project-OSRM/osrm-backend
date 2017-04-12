# 5.7.0
  - Changes from 5.6
    - Algorithm:
      - OSRM object has new option `algorithm` that allows the selection of a routing algorithm.
      - New experimental algorithm: Multi-Level Dijkstra with new toolchain:
        - Allows for fast metric updates in below a minute on continental sized networks (osrm-customize)
        - Plugins supported: `match` and `route`
        - Quickstart: `osrm-extract data.osm.pbf`, `osrm-partition data.osrm`, `osrm-customize data.osrm`, `osrm-routed --algorithm=MLD data.osrm`
    - NodeJs Bindings
      - Merged https://github.com/Project-OSRM/node-osrm into repository. Build via `cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_NODE_BINDINGS=On -DENABLE_MASON=On`.
      - `OSRM` object has new option `algorihtm="CH","CoreCH","MLD"`
    - Internals
      - Shared memory notification via conditional variables on Linux or semaphore queue on OS X and Windows with a limit of 128 OSRM Engine instances
    - Files
      - .osrm.datasource_index file was removed. Data is now part of .osrm.geometries.
      - .osrm.edge_lookup was removed. The option `--generate-edge-lookup` does nothing now.
      - `osrm-contract` does not depend on the `.osrm.fileIndex` file anymore
      - `osrm-extract` creates new file `.osrm.cnbg` and `.cnbg_to_ebg`
      - `osrm-partition` creates new file `.osrm.partition` and `.osrm.cells`
      - `osrm-customize` creates new file `.osrm.mldgr`
    - Profiles
      - Added `force_split_edges` flag to global properties. True value guarantees that segment_function will be called for all segments, but also could double memory consumption
    - Map Matching:
      - new option `gaps=split|ignore` to enable/disbale track splitting
      - new option `tidy=true|false` to simplify traces automatically

# 5.6.3
  - Changes from 5.6.0
    - Bugfixes
      - #3790 Fix incorrect speed values in tile plugin

# 5.6.2
  - Changes from 5.6.0
    - Bugfixes
      - Fix incorrect forward datasources getter in facade

# 5.6.1
  - Changes from 5.6.0
    - Bugfixes
      - Fix #3754 add restricted penalty on NoTurn turns

# 5.6.0
  - Changes from 5.5
    - Bugfixes
      - Fix #3475 removed an invalid `exit` field from the `arrive` maneuver
      - Fix #3515 adjusted number of `nodes` in `annotation`
      - Fix #3605 Fixed a bug that could lead to turns at the end of the road to be suppressed
      - Fix #2844 handle up to 16777215 code units in OSM names
    - Infrastructure
      - Support building rpm packages.
    - Guidance
      - No longer emitting turns on ferries, if a ferry should use multiple docking locations
    - Profiles
      - Removed the `./profile.lua -> ./profiles/car.lua` symlink. Use specific profiles from the `profiles` directory.
      - `properties` object has a new `weight_name` field, default value is "duration"
      - `properties` object has a new `weight_precision` field that specifies a decimal precision of edge weights, default value 1
      - In `way_function` the filed `forward_rate` and `backward_rate` of `ExtractionWay` can now be set.
        They have the same interpretation for the way weight as `forward_speed` and `backward_speed` for the edge duration.
        The unit of rate is meters per weight unit, so higher values will be prefered during routing.
      - `turn_function` now does not return an integer but takes in a `ExtractionTurn` object and can modify the `weight` and `duration` fields
      - `segment_function` now takes in a `ExtractionSegment` object and can modify `weight` and `duration` fields
      - `properties.uturn_penalty` is deprecated. Set it in the `turn_function`. The turn type is exposed as `ExtractionTurn::direction_modifier`.
      - `properties.traffic_light_penalty` is deprecated. Traffic light penalties now need to be set over in the turn function.
         Each turn with a traffic light is marked with `ExtractionTurn::has_traffic_light = true`.
      - Renamed the helper file `profiles/lib/directional.lua` to `profiles/lib/tags.lua` since it now provides more general tags parsing utility functions.
      - The car and foot profiles now depend on the helper file `profiles/lib/handlers.lua`.
    - Infrastructure
      - Disabled link-time optimized (LTO) builds by default. Enable by passing `-DENABLE_LTO=ON` to `cmake` if you need the performance and know what you are doing.
      - Datafile versioning is now based on OSRM semver values, rather than source code checksums.
        Datafiles are compatible between patch levels, but incompatible between minor version or higher bumps.
      - libOSRM now creates an own watcher thread then used in shared memory mode to listen for data updates
    - Tools:
      - Added osrm-extract-conditionals tool for checking conditional values in OSM data
    - Trip Plugin
      - Added a new feature that finds the optimal route given a list of waypoints, a source and a destination. This does not return a roundtrip and instead returns a one way optimal route from the fixed source to the destination points.

# 5.5.1
  - Changes from 5.5.0
    - API:
      - Adds `generate_hints=true` (`true` by default) which lets user disable `Hint` generating in the response. Use if you don't need `Hint`s!
    - Bugfixes
      - Fix #3418 and ensure we only return bearings in the range 0-359 in API responses
      - Fixed a bug that could lead to emitting false instructions for staying on a roundabout

# 5.5.0
  - Changes from 5.4.0
    - API:
      - `osrm-datastore` now accepts the parameter `--max-wait` that specifies how long it waits before aquiring a shared memory lock by force
      - Shared memory now allows for multiple clients (multiple instances of libosrm on the same segment)
      - Polyline geometries can now be requested with precision 5 as well as with precision 6
    - Profiles
      - the car profile has been refactored into smaller functions
      - get_value_by_key() is now guaranteed never to return empty strings, nil is returned instead.
      - debug.lua was added to make it easier to test/develop profile code.
      - `car.lua` now depends on lib/set.lua and lib/sequence.lua
      - `restrictions` is now used for namespaced restrictions and restriction exceptions (e.g. `restriction:motorcar=` as well as `except=motorcar`)
      - replaced lhs/rhs profiles by using test defined profiles
      - Handle `oneway=alternating` (routed over with penalty) separately from `oneway=reversible` (not routed over due to time dependence)
      - Handle `destination:forward`, `destination:backward`, `destination:ref:forward`, `destination:ref:backward` tags
      - Properly handle destinations on `oneway=-1` roads
    - Guidance
      - Notifications are now exposed more prominently, announcing turns onto a ferry/pushing your bike more prominently
      - Improved turn angle calculation, detecting offsets due to lanes / minor variations due to inaccuracies
      - Corrected the bearings returned for intermediate steps - requires reprocessing
      - Improved turn locations for collapsed turns
      - Sliproad classification refinements: the situations we detect as Sliproads now resemble more closely the reality
    - Trip Plugin
      - changed internal behaviour to prefer the smallest lexicographic result over the largest one
    - Bugfixes
      - fixed a bug where polyline decoding on a defective polyline could end up in out-of-bound access on a vector
      - fixed compile errors in tile unit-test framework
      - fixed a bug that could result in inconsistent behaviour when collapsing instructions
      - fixed a bug that could result in crashes when leaving a ferry directly onto a motorway ramp
      - fixed a bug in the tile plugin that resulted in discovering invalid edges for connections
      - improved error messages when missing files during traffic updates (#3114)
      - For single coordinate geometries the GeoJSON `Point` encoding was broken. We now always emit `LineString`s even in the one-coordinate-case (backwards compatible) (#3425)
    - Debug Tiles
      - Added support for turn penalties
    - Internals
      - Internal/Shared memory datafacades now share common memory layout and data loading code
      - File reading now has much better error handling
    - Misc
      - Progress indicators now print newlines when stdout is not a TTY
      - Prettier API documentation now generated via `npm run build-api-docs` output `build/docs`

# 5.4.3
  - Changes from 5.4.2
    - Bugfixes
      - #3254 Fixed a bug that could end up hiding roundabout instructions
      - #3260 fixed a bug that provided the wrong location in the arrival instruction

# 5.4.2
  - Changes from 5.4.1
    - Bugfixes
      - #3032 Fixed a bug that could result in emitting `invalid` as an instruction type on sliproads with mode changes
      - #3085 Fixed an outdated assertion that could throw without a cause for concern
      - #3179 Fixed a bug that could trigger an assertion in TurnInstruciton generation

# 5.4.1
  - Changes from 5.4.0
    - Bugfixes
      - #3016: Fixes shared memory updates while queries are running

# 5.4.0
  - Changes from 5.3.0
    - Profiles
      - includes library guidance.lua that offers preliminary configuration on guidance.
      - added left_hand_driving flag in global profile properties
      - modified turn penalty function for car profile - better fit to real data
      - return `ref` and `name` as separate fields. Do no use ref or destination as fallback for name value
      - the default profile for car now ignores HOV only roads
    - Guidance
      - Handle Access tags for lanes, only considering valid lanes in lane-guidance (think car | car | bike | car)
      - Improved the detection of non-noticeable name-changes
      - Summaries have been improved to consider references as well
    - API:
      - `annotations=true` now returns the data source id for each segment as `datasources`
      - Reduced semantic of merge to refer only to merges from a lane onto a motorway-like road
      - new `ref` field in the `RouteStep` object. It contains the reference code or name of a way. Previously merged into the `name` property like `name (ref)` and are now separate fields.
    - Bugfixes
      - Fixed an issue that would result in segfaults for viaroutes with an invalid intermediate segment when u-turns were allowed at the via-location
      - Invalid only_* restrictions could result in loss of connectivity. As a fallback, we assume all turns allowed when the restriction is not valid
      - Fixed a bug that could result in an infinite loop when finding information about an upcoming intersection
      - Fixed a bug that led to not discovering if a road simply looses a considered prefix
      - BREAKING: Fixed a bug that could crash postprocessing of instructions on invalid roundabout taggings. This change requires reprocessing datasets with osrm-extract and osrm-contract
      - Fixed an issue that could emit `invalid` as instruction when ending on a sliproad after a traffic-light
      - Fixed an issue that would detect turning circles as sliproads
      - Fixed a bug where post-processing instructions (e.g. left + left -> uturn) could result in false pronunciations
      - Fixes a bug where a bearing range of zero would cause exhaustive graph traversals
      - Fixes a bug where certain looped geometries could cause an infinite loop during extraction
      - Fixed a bug where some roads could be falsly identified as sliproads
      - Fixed a bug where roundabout intersections could result in breaking assertions when immediately exited
    - Infrastructure:
      - Adds a feature to limit results in nearest service with a default of 100 in `osrm-routed`

# 5.3.0
  - Changes from 5.3.0-rc.3
    - Guidance
      - Only announce `use lane` on required turns (not using all lanes to go straight)
      - Moved `lanes` to the intersection objects. This is BREAKING in relation to other Release Candidates but not with respect to other releases.
    - Bugfixes
      - Fix BREAKING: bug that could result in failure to load 'osrm.icd' files. This breaks the dataformat
      - Fix: bug that results in segfaults when `use lane` instructions are suppressed

  - Changes form 5.2.7
    - API
      - Introduces new `TurnType` in the form of `use lane`. The type indicates that you have to stick to a lane without turning
      - Introduces `lanes` to the `Intersection` object. The lane data contains both the markings at the intersection and a flag indicating if they can be chosen for the next turn
      - Removed unused `-s` from `osrm-datastore`
    - Guidance
      - Only announce `use lane` on required turns (not using all lanes to go straight)
      - Improved detection of obvious turns
      - Improved turn lane detection
      - Reduce the number of end-of-road instructions in obvious cases
    - Profile:
      - bicycle.lua: Surface speeds never increase the actual speed
    - Infrastructure
      - Add 32bit support
      - Add ARM NEON/VFP support
      - Fix Windows builds
      - Optimize speed file updates using mmap
      - Add option to disable LTO for older compilers
      - BREAKING: The new turn type changes the turn-type order. This breaks the **data format**.
      - BREAKING: Turn lane data introduces two new files (osrm.tld,osrm.tls). This breaks the fileformat for older versions.
    - Bugfixes:
      - Fix devide by zero on updating speed data using osrm-contract

# 5.3.0 RC3
  - Changes from 5.3.0-rc.2
    - Guidance
      - Improved detection of obvious turns
      - Improved turn lane detection
    - Bugfixes
      - Fix bug that didn't chose minimal weights on overlapping edges

# 5.3.0 RC2
  - Changes from 5.3.0-rc.1
    - Bugfixes
      - Fixes invalid checks in the lane-extraction part of the car profile

# 5.3.0 RC1
    - API
     - Introduces new `TurnType` in the form of `use lane`. The type indicates that you have to stick to a lane without turning
     - Introduces lanes to the route response. The lane data contains both the markings at the intersection and a flag indicating their involvement in the turn

    - Infrastructure
     - BREAKING: The new turn type changes the turn-type order. This breaks the **data format**.
     - BREAKING: Turn lane data introduces two new files (osrm.tld,osrm.tls). This breaks the fileformat for older versions.

# 5.2.5
  - Bugfixes
    - Fixes a segfault caused by incorrect trimming logic for very short steps.

# 5.2.4
  - Bugfixes:
    - Fixed in issue that arised on roundabouts in combination with intermediate intersections and sliproads

# 5.2.3
  - Bugfixes:
    - Fixed an issue with name changes in roundabouts that could result in crashes

# 5.2.2
  Changes from 5.2.1
  - Bugfixes:
    - Buffer overrun in tile plugin response handling

# 5.2.1
  Changes from 5.2.0
  - Bugfixes:
    - Removed debug statement that was spamming the console

# 5.2.0
  Changes from 5.2.0 RC2
   - Bugfixes:
     - Fixed crash when loading shared memory caused by invalid OSM IDs segment size.
     - Various small instructions handling fixes

   Changes from 5.1.0
   - API:
     - new parameter `annotations` for `route`, `trip` and `match` requests.  Returns additional data about each
       coordinate along the selected/matched route line per `RouteLeg`:
         - duration of each segment
         - distance of each segment
         - OSM node ids of all segment endpoints
     - Introducing Intersections for Route Steps. This changes the API format in multiple ways.
         - `bearing_before`/`bearing_after` of `StepManeuver` are now deprecated and will be removed in the next major release
         - `location` of `StepManeuvers` is now deprecated and will be removed in the next major release
         - every `RouteStep` now has property `intersections` containing a list of `Intersection` objects.
     - Support for destination signs. New member `destinations` in `RouteStep`, based on `destination` and `destination:ref`
     - Support for name pronunciations. New member `pronunciation` in `RouteStep`, based on `name:pronunciation`

   - Profile changes:
     - duration parser now accepts P[n]DT[n]H[n]M[n]S, P[n]W, PTHHMMSS and PTHH:MM:SS ISO8601 formats.
     - `result.destinations` allows you to set a way's destinations
     - `result.pronunciation` allows you to set way name pronunciations
     - `highway=motorway_link` no longer implies `oneway` as per the OSM Wiki

   - Infrastructure:
     - BREAKING: Changed the on-disk encoding of the StaticRTree to reduce ramIndex file size. This breaks the **data format**
     - BREAKING: Intersection Classification adds a new file to the mix (osrm.icd). This breaks the fileformat for older versions.
     - Better support for osrm-routed binary upgrade on the fly [UNIX specific]:
       - Open sockets with SO_REUSEPORT to allow multiple osrm-routed processes serving requests from the same port.
       - Add SIGNAL_PARENT_WHEN_READY environment variable to enable osrm-routed signal its parent with USR1 when it's running and waiting for requests.
     - Disable http access logging via DISABLE_ACCESS_LOGGING environment variable.

   - Guidance:
     - BREAKING: modifies the file format with new internal identifiers
     - improved detection of turning streets, not reporting new-name in wrong situations
     - improved handling of sliproads (emit turns instead of 'take the ramp')
     - improved collapsing of instructions. Some 'new name' instructions will be suppressed if they are without alternative and the segment is short

   - Bugfixes
     - fixed broken summaries for very short routes

# 5.2.0 RC2
   Changes from 5.2.0 RC1

   - Guidance:
     - improved handling of sliproads (emit turns instead of 'take the ramp')
     - improved collapsing of instructions. Some 'new name' instructions will be suppressed if they are without alternative and the segment is short
     - BREAKING: modifies the file format with new internal identifiers

   - API:
     - paramater `annotate` was renamed to `annotations`.
     - `annotation` as accidentally placed in `Route` instead of `RouteLeg`
     - Support for destination signs. New member `destinations` in `RouteStep`, based on `destination` and `destination:ref`
     - Support for name pronunciations. New member `pronunciation` in `RouteStep`, based on `name:pronunciation`
     - Add `nodes` property to `annotation` in `RouteLeg` containing the ids of nodes covered by the route

   - Profile changes:
     - `result.destinations` allows you to set a way's destinations
     - `result.pronunciation` allows you to set way name pronunciations
     - `highway=motorway_link` no longer implies `oneway` as per the OSM Wiki

   - Infrastructure
     - BREAKING: Changed the on-disk encoding of the StaticRTree to reduce ramIndex file size. This breaks the **data format**

   - Bugfixes
     - fixed broken summaries for very short routes

# 5.2.0 RC1
   Changes from 5.1.0

   - API:
     - new parameter `annotate` for `route` and `match` requests.  Returns additional data about each
       coordinate along the selected/matched route line.
     - Introducing Intersections for Route Steps. This changes the API format in multiple ways.
         - `bearing_before`/`bearing_after` of `StepManeuver` are now deprecated and will be removed in the next major release
         - `location` of `StepManeuvers` is now deprecated and will be removed in the next major release
         - every `RouteStep` now has property `intersections` containing a list of `Intersection` objects.

   - Profile changes:
     - duration parser now accepts P[n]DT[n]H[n]M[n]S, P[n]W, PTHHMMSS and PTHH:MM:SS ISO8601 formats.

   - Infrastructure:
     - Better support for osrm-routed binary upgrade on the fly [UNIX specific]:
       - Open sockets with SO_REUSEPORT to allow multiple osrm-routed processes serving requests from the same port.
       - Add SIGNAL_PARENT_WHEN_READY environment variable to enable osrm-routed signal its parent with USR1 when it's running and waiting for requests.
     - BREAKING: Intersection Classification adds a new file to the mix (osrm.icd). This breaks the fileformat for older versions.
     - Disable http access logging via DISABLE_ACCESS_LOGGING environment
       variable.

   - Guidance:
     - improved detection of turning streets, not reporting new-name in wrong situations

# 5.1.0
   Changes with regard to 5.0.0

   - API:
     - added StepManeuver type `roundabout turn`. The type indicates a small roundabout that is treated as an intersection
        (turn right at the roundabout for first exit, go straight at the roundabout...)
     - added StepManeuver type `on ramp` and `off ramp` to distinguish between ramps that enter and exit a highway.
     - reduced new name instructions for trivial changes
     - combined multiple turns into a single instruction at segregated roads`

   - Profile Changes:
    - introduced a suffix_list / get_name_suffix_list to specify name suffices to be suppressed in name change announcements
    - street names are now consistently assembled for the car, bike and walk profile as: "Name (Ref)" as in "Berlin (A5)"
    - new `car.lua` dependency `lib/destination.lua`
    - register a way's .nodes() function for use in the profile's way_function.

   - Infrastructure
    - BREAKING: reordered internal instruction types. This breaks the **data format**
    - BREAKING: Changed the on-disk encoding of the StaticRTree for better performance. This breaks the **data format**

   - Fixes:
    - Issue #2310: post-processing for local paths, fixes #2310
    - Issue #2309: local path looping, fixes #2309
    - Issue #2356: Make hint values optional
    - Issue #2349: Segmentation fault in some requests
    - Issue #2335: map matching was using shortest path with uturns disabled
    - Issue #2193: Fix syntax error position indicators in parameters queries
    - Fix search with u-turn
    - PhantomNode packing in MSVC now the same on other platforms
    - Summary is now not malformed when including unnamed roads
    - Emit new-name on when changing fron unanmed road to named road

# 5.0.0
   Changes with regard 5.0.0 RC2:
   - API:
     - if `geometry=geojson` is passed the resulting geometry can be a LineString or Point
       depending on how many coordinates are present.
     - the removal of the summary field was revered. for `steps=flase` the field will always be an empty string.

   Changes with regard to 4.9.1:
   - API:
     - BREAKING: Complete rewrite of the HTTP and library API. See detailed documentation in the wiki.
     - BREAKING: The default coordinate order is now `longitude, latidue`. Exception: Polyline geometry
         which follow the original Google specification of `latitdue, longitude`.
     - BREAKING: Polyline geometries now use precision 5, instead of previously 6
     - BREAKING: Removed GPX support
     - New service `tile` which serves debug vector tiles of the road network
     - Completely new engine for guidance generation:
        - Support for highway ramps
        - Support for different intersection types (end of street, forks, merges)
        - Instruction post-processing to merge unimportant instructions
        - Improved handling of roundabouts

   - Tools:
     - BREAKING: Renamed osrm-prepare to osrm-contract
     - BREAKING: Removes profiles from osrm-contract, only needed in osrm-extract.
     - Abort processing in osrm-extract if there are no snappable edges remaining.
     - Added .properties file to osrm-extract ouput.
     - Enables the use of multiple segment-speed-files on the osrm-contract command line

   - Profile changes:
     - Remove movable bridge mode
     - Add `maxspeed=none` tag to car profile.
     - A `side_road` tag support for the OSRM car profile.

   - Fixes:
     - Issue #2150: Prevents routing over delivery ways and nodes
     - Issue #1972: Provide uninstall target
     - Issue #2072: Disable alternatives by default and if core factor < 1.0
     - Issue #1999: Fix unpacking for self-loop nodes not in core.

   - Infrastructure:
     - Cucumber test suit is now based on cucumber-js, removes Ruby as dependency
     - Updated to mapbox/variant v1.1
     - Updated to libosmium v2.6.1
     - Remove GeoJSON based debugging output, replaced by debug tiles


# 5.0.0 RC2
   - Profiles:
      - `properties.allow_uturns_at_via` -> `properties.continue_straight_at_waypoint` (value is inverted!)
   - API:
      - Removed summary from legs property
      - Disable steps and alternatives by default
      - Fix `code` field: 'ok' -> 'Ok'
      - Allow 4.json and 4.3.json format
      - Conform to v5 spec and support "unlimited" as radiuses value.
      - `uturns` parameter was replaced by `continue_straight` (value is inverted!)
   - Features:
      - Report progress for gennerating edge expanded edges in the edge based graph factory
      - Add maxspeed=none tag to car profile.
      - Optimize StaticRTree code: speedup 2x (to RC1)
      - Optimize DouglasPeucker code: speedup 10x (to RC1)
      - Optimize WebMercator projection: speedup 2x (to RC1)
   - Bugs:
      - #2195: Resolves issues with multiple includedirs in pkg-config file
      - #2219: Internal server error when using the match plugin
      - #2027: basename -> filename
      - #2168: Report correct position where parsing failed
      - #2036: Add license to storage and storage config exposed in public API
      - Fix uturn detection in match plugin
      - Add missing -lz to fix linking of server-tests

# 5.0.0 RC1
   - Renamed osrm-prepare into osrm-contract
   - osrm-contract does not need a profile parameter anymore
   - New public HTTP API, find documentation [here](https://github.com/Project-OSRM/osrm-backend/wiki/New-Server-api)
   - POST support is discontinued, please use library bindings for more complex requests
   - Removed timestamp plugin
   - Coordinate order is now Longitude,Latitude
   - Cucumber tests now based on Javascript (run with `npm test`)
   - Profile API changed:
      - `forward_mode` and `backward_mode` now need to be selected from a pre-defined list
      - Global profile properties are now stored in a global `properties` element. This includes:
        - `properties.traffic_signal_penalty`
        - `properties.use_turn_restrictions`
        - `properties.u_turn_penalty`
        - `properties.allow_u_turn_at_via`
