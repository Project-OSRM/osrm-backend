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

## way_function

Given an OpenStreetMap way, the way_function will either return nothing (meaning we are not going to route over this way at all), or it will set up a result hash to be returned. The most important thing it will do is set the value of `result.forward_speed` and `result.backward_speed` as a suitable integer value representing the speed for traversing the way.

All other calculations stem from that, including the returned timings in driving directions, but also, less directly, it feeds into the actual routing decisions the engine will take (a way with a slow traversal speed, may be less favoured than a way with fast traversal speed, but it depends how long it is, and... what it connects to in the rest of the network graph)

Using the power of the scripting language you wouldn't typically see something as simple as a `result.forward_speed = 20` line within the way_function. Instead a way_function will examine the tagging (e.g. `way:get_value_by_key("highway")` and many others), process this information in various ways, calling other local functions, referencing the global variables and look-up hashes, before arriving at the result.

## Guidance

The guidance parameters in profiles are currently a work in progress. They can and will change.
Please be aware of this when using guidance configuration possibilities.

### Road Classification

Guidance uses road classes to decide on when/if to emit specific instructions and to discover which road is obvious when following a route.
Classification uses three flags and a priority-category.
The flags indicate whether a road is a motorway (required for on/off ramps), a link type (the ramps itself, if also a motorway) and whether a road may be omittted in considerations (is considered purely for connectivity).
The priority-category influences the decision which road is considered the obvious choice and which roads can be seen as fork.
Forks can be emitted between roads of similar priority category only. Obvious choices follow a major priority road, if the priority difference is large.
