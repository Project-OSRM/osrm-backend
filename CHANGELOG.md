# 5.4.0
  Changes from 5.3.0
    - Profiles
      - includes library guidance.lua that offers preliminary configuration on guidance.
      - added left_hand_driving flag in global profile properties
    - Guidance
      - Handle Access tags for lanes, only considering valid lanes in lane-guidance (think car | car | bike | car)
    - API:
      - `annotations=true` now returns the data source id for each segment as `datasources`
      - Reduced semantic of merge to refer only to merges from a lane onto a motorway-like road
    - Bugfixes
      - Fixed an issue that would result in segfaults for viaroutes with an invalid intermediate segment when u-turns were allowed at the via-location

# 5.3.0
  Changes from 5.3.0-rc.3
    - Guidance
      - Only announce `use lane` on required turns (not using all lanes to go straight)
      - Moved `lanes` to the intersection objects. This is BREAKING in relation to other Release Candidates but not with respect to other releases.
    - Bugfixes
      - Fix BREAKING: bug that could result in failure to load 'osrm.icd' files. This breaks the dataformat
      - Fix: bug that results in segfaults when `use lane` instructions are suppressed

  Changes form 5.2.7
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
  Changes from 5.3.0-rc.2
    - Guidance
      - Improved detection of obvious turns
      - Improved turn lane detection
    - Bugfixes
      - Fix bug that didn't chose minimal weights on overlapping edges

# 5.3.0 RC2
  Changes from 5.3.0-rc.1
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


