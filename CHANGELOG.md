# 5.0.0 RC2
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


