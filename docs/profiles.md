OSRM supports "profiles". Configurations representing different routing behaviours for (typically) different transport modes. A profile describes whether or not we route along a particular type of way, or over a particular node in the OpenStreetMap data, and also how quickly we'll be travelling when we do. This feeds into the way the routing graph is created and thus influences the output routes.

## Available profiles

Out-of-the-box OSRM comes with several different profiles, including car, bicycle and foot.

Profile configuration files have a 'lua' extension, and are found under the 'profiles' subdirectory.
Alternatively commands will take a lua profile specified with an explicit -p param, for example:

`osrm-extract -p ../profiles/car.lua planet-latest.osm.pbf`

And then **you will need to extract and contract again** (A change to the profile will typically affect the extract step as well as the contract step. See [Processing Flow](https://github.com/Project-OSRM/osrm-backend/wiki/Processing-Flow))

## lua scripts?

Profiles are not just configuration files. They are scripts written in the "lua" scripting language ( http://www.lua.org )  The reason for this, is that OpenStreetMap data is not sufficiently straightforward, to simply define tag mappings. Lua scripting offers a powerful way of coping with the complexity of different node,way,relation,tag combinations found within OpenStreetMap data.

## Basic structure of a profile

You can understand these lua scripts enough to make interesting modifications, without needing to get to grips with how they work completely.

Towards the top of the file, a profile (such as [car.lua](../profiles/car.lua)) will typically define various configurations as global variables. A lot of these are look-up hashes of one sort or another.

As you scroll down the file you'll see local variables, and then local functions, and finally...

`way_function` and `node_function` are the important functions which are called when extracting OpenStreetMap data with `osrm-extract`.

The following global properties can be set in your profile:

Attribute                     | Type     | Notes
------------------------------|----------|----------------------------------------------------------------------------
weight_name                   | String   | Name used in output for the routing weight property (default 'duration')
weight_precision              | Unsigned | Decimal precision of edge weights (default 1)
left_hand_driving             | Boolean  | Are vehicles assumed to drive on the left? (used in guidance)
use_turn_restrictions         | Boolean  | Are turn instructions followed?
continue_straight_at_waypoint | Boolean  | Must the route continue straight on at a via point, or are U-turns allowed?
max_speed_for_map_matching    | Float    | Maximum vehicle speed to be assumed in matching (in m/s)
max_turn_weight               | Float    | Maximum turn penalty weight
force_split_edges             | Boolean  | True value forces a split of forward and backward edges of extracted ways and guarantees that segment_function will be called for all segments

## way_function

Given an OpenStreetMap way, the way_function will either return nothing (meaning we are not going to route over this way at all), or it will set up a result hash to be returned. The most important thing it will do is set the value of `result.forward_speed` and `result.backward_speed` as a suitable integer value representing the speed for traversing the way.

All other calculations stem from that, including the returned timings in driving directions, but also, less directly, it feeds into the actual routing decisions the engine will take (a way with a slow traversal speed, may be less favoured than a way with fast traversal speed, but it depends how long it is, and... what it connects to in the rest of the network graph)

Using the power of the scripting language you wouldn't typically see something as simple as a `result.forward_speed = 20` line within the way_function. Instead a way_function will examine the tagging (e.g. `way:get_value_by_key("highway")` and many others), process this information in various ways, calling other local functions, referencing the global variables and look-up hashes, before arriving at the result.

The following attributes can be set on the result in way_function:

Attribute                               | Type     | Notes
----------------------------------------|----------|--------------------------------------------------------------------------
forward_speed                           | Float    | Speed on this way in km/h
backward_speed                          | Float    |  "   "
forward_rate                            | Float    | Routing weight, expressed as meters/*weight* (e.g. for a fastest-route weighting, you would want this to be meters/second, so set it to forward_speed/3.6)
backward_rate                           | Float    |  "   "
forward_mode                            | Enum     | Mode of travel (e.g. car, ferry). Defined in include/extractor/travel_mode.hpp
backward_mode                           | Enum     |  "   "
duration                                | Float    | Alternative setter for duration of the whole way in both directions
weight                                  | Float    | Alternative setter for weight of the whole way in both directions
turn_lanes_forward                      | String   | Directions for individual lanes (normalised OSM turn:lanes value)
turn_lanes_backward                     | String   |  "   "
forward_restricted                      | Boolean  | Is this a restricted access road? (e.g. private, or deliveries only; used to enable high turn penalty, so that way is only chosen for start/end of route)
backward_restricted                     | Boolean  |  "   "
is_startpoint                           | Boolean  | Can a journey start on this way? (e.g. ferry; if false, prevents snapping the start point to this way)
roundabout                              | Boolean  | Is this part of a roundabout?
circular                                | Boolean  | Is this part of a non-roundabout circular junction?
name                                    | String   | Name of the way
ref                                     | String   | Road number
pronunciation                           | String   | Name pronunciation
road_classification.motorway_class      | Boolean  | Guidance: way is a motorway
road_classification.link_class          | Boolean  | Guidance: way is a slip/link road
road_classification.road_priority_class | Enum     | Guidance: order in priority list. Defined in include/extractor/guidance/road_classification.hpp
road_classification.may_be_ignored      | Boolean  | Guidance: way is non-highway
road_classification.num_lanes           | Unsigned | Guidance: total number of lanes in way

### Guidance

The guidance parameters in profiles are currently a work in progress. They can and will change.
Please be aware of this when using guidance configuration possibilities.

Guidance uses road classes to decide on when/if to emit specific instructions and to discover which road is obvious when following a route.
Classification uses three flags and a priority-category.
The flags indicate whether a road is a motorway (required for on/off ramps), a link type (the ramps itself, if also a motorway) and whether a road may be omittted in considerations (is considered purely for connectivity).
The priority-category influences the decision which road is considered the obvious choice and which roads can be seen as fork.
Forks can be emitted between roads of similar priority category only. Obvious choices follow a major priority road, if the priority difference is large.

## node_function

The following attributes can be set on the result in node_function:

Attribute       | Type    | Notes
----------------|---------|-------------------------------------------------------
barrier         | Boolean | Is it an impassable barrier?
traffic_lights  | Boolean | Is it a traffic light (incurs delay in turn_function)?

## segment_function

The following attributes can be read and set on the result in segment_function:

Attribute          | Read/write? | Type    | Notes
-------------------|-------------|---------|------------------------------------------------------
source.lon         | Read        | Float   | Co-ordinates of segment start
source.lat         | Read        | Float   |  "   "
target.lon         | Read        | Float   | Co-ordinates of segment end
target.lat         | Read        | Float   |  "   "
target.distance    | Read        | Float   | Length of segment
weight             | Read/write  | Float   | Routing weight for this segment
duration           | Read/write  | Float   | Duration for this segment

## turn_function

The following attributes can be read and set on the result in turn_function:

Attribute          | Read/write? | Type    | Notes
-------------------|-------------|---------|------------------------------------------------------
direction_modifier | Read        | Enum    | Geometry of turn. Defined in include/extractor/guidance/turn_instruction.hpp
turn_type          | Read        | Enum    | Priority of turn. Defined in include/extractor/guidance/turn_instruction.hpp
has_traffic_light  | Read        | Boolean | Is a traffic light present at this turn?
source_restricted  | Read        | Boolean | Is it from a restricted access road? (See definition in way_function)
target_restricted  | Read        | Boolean | Is it to a restricted access road? (See definition in way_function)
angle              | Read        | Float   | Angle of turn in degrees (0-360: 0=u-turn, 180=straight on)
duration           | Read/write  | Float   | Penalty to be applied for this turn (duration in deciseconds)
weight             | Read/write  | Float   | Penalty to be applied for this turn (routing weight)
