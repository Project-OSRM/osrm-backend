# How to route inside pedestrian areas {#pedestrian_areas}

How to route inside pedestrian areas, or over the interior of an area where you can
travel freely in all directions.

%OSRM can create routes crossing the interior of an area by generating virtual ways
between every pair of entry points to the area. This process is called @em meshing. The
generated ways follow lines of sight, avoid obstacles, and use existing nodes. An entry
point is where another way connects to the perimeter of the area.

This feature is still EXPERIMENTAL.

## Configuration

To opt-in to this feature, you must declare an algorithm to be used for area meshing.
Find your LUA profile's @ref setup function and insert this line:

```lua
function setup()
  ...
  area_manager:init('visgraph+dijkstra')
  ...
end
```

Note: Only the `visgraph+dijkstra` algorithm is available at present.

All areas to be meshed must be registered with the @ref AreaManager. In OpenStreetMap <a
href="https://wiki.openstreetmap.org/wiki/Tag:highway%3Dpedestrian#Squares_and_plazas">
areas are mapped</a> either as a closed way or as a multipolygon relation. Both flavours
must be configured separately.

### Meshing closed ways

To mesh a closed way you must register it in your @ref process_way function. Insert
following lines into your existing `process_way` function, immediately after the "quick
initial test":

```lua
function process_way(profile, way, result, relations)
  ...
  if way:has_tag('highway', 'pedestrian') and way:has_true_tag('area') then
    -- register the way
    area_manager:way(way)
    return
  end
  ...
end
```

(Note that open ways cannot be meshed and will be ignored.)

### Meshing multipolygon relations

To mesh a multipolygon relation you must register it in the @ref process_relation
function. The `process_relation` function is a newly introduced function that is called
for every relation in the input file. You'll have to create the function like this:

```lua
function process_relation(profile, relation, relations)
  if relation:has_tag('type', 'multipolygon') and relation:has_tag('highway', 'pedestrian') then
    -- register the relation
    area_manager:relation(relation)
  end
end
```

And you must also return the `process_relation` function at the end of your profile:

```lua
return {
  setup = setup,
  process_way =  process_way,
  process_node = process_node,
  process_relation = process_relation, -- << add this line
  ...
}
```

At this point you have a working basic configuration. Remember that you must run
`osrm-extract` before your changes become effective.

### Processing the generated ways

While not necessary, you may want to apply further processing to the @em generated ways.
The generated ways are passed to the @ref process_way function in the usual fashion.
They have the same tags as the original way or relation, except:

- the `area` tag is removed on ways,
- the `type` tag is removed on relations,
- an `osrm:virtual=yes` tag is added.

You can pick generated ways like this:

```lua
function process_way(profile, way, result, relations)
  ...
  if way:has_key('osrm:virtual') then
    -- do something with the way here
  end
  ...
end
```

## Known trade-offs and follow-ups

The feature ships EXPERIMENTAL, and a few deliberate trade-offs were left in place to
keep the initial change reviewable. They are recorded here rather than in scattered
TODOs so that whoever picks them up can see the reasoning.

**The sweep tests every edge of the status instead of only the nearest one.** The
textbook keeps the sweep status ordered by distance along the sweep ray and tests only
its nearest edge. That ordering cannot be maintained here: whenever the ray runs exactly
through a vertex the ray/segment intersection is rejected as an endpoint hit and the edge
keeps a stale distance, which is enough to put a non-blocking edge in front and report a
blocked vertex as visible. Testing the whole status answers the same question without
depending on the distances, at a cost that is irrelevant because the status only ever
holds the few edges the ray currently crosses. See VisibilityGraph.

**Boost.Geometry is still used for the geometry plumbing.** `bg::distance()` in
AreaMesher::run_dijkstra could use %OSRM's own haversine, and the Mercator projection
could use `util::web_mercator` -- which would additionally remove a global object living
in an anonymous namespace in a header. Note that `bg::distance()` returns *radians* while
`coordinate_calculation` returns *metres*: the swap rescales every edge weight by the
earth radius, which leaves shortest paths unchanged but changes what
`Dijkstra::distance_epsilon` -- an absolute epsilon -- considers a tie. That epsilon has
to be reconsidered in the same change. The polygon and ring models, the traits
adaptations and the ~56 `boost::geometry::get()` accessor calls every geometry predicate
is written against are a deeper dependency and are not worth unpicking.

**The area module has its own priority queue.** See IndexPriorityQueue for why, and for
what would have to change to use `util::QueryHeap` instead.

**The visibility graph has no unit tests.** VisibilityGraph defines its member functions
in the header without `inline`, so the header can only ever be included by a single
translation unit -- today `area_mesher.cpp`. Since the unit test binary links
`osrm_extract`, a test translation unit including the header would be a duplicate-symbol
link error. Moving the definitions into a `src/extractor/area/visibility_graph.cpp` would
fix that, remove the same-header anonymous-namespace projection object, and let the
differential test that was used to validate the sweep (comparing it against a brute-force
reference over random polygons with holes) live in the repository.

@sa AreaManager
<br> A complete example profile is found in the file: [profiles/foot_area.lua](../profiles/foot_area.lua).
<br> https://wiki.openstreetmap.org/wiki/Relation:multipolygon
<br> https://wiki.openstreetmap.org/wiki/Key:area
<br> https://wiki.openstreetmap.org/wiki/Tag:highway%3Dpedestrian#Squares_and_plazas
