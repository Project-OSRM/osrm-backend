# How to route inside pedestrian areas {#pedestrian_areas}

How to route inside pedestrian areas, or over the interior of an area where you can
travel freely in all directions.

%OSRM can create routes crossing the interior of an area by generating virtual ways along
its lines of sight. This process is called @em meshing. The generated ways avoid obstacles
and use existing nodes.

The mesh is pruned: of the whole visibility graph, %OSRM keeps the shortest-path tree
rooted at each entry point, plus every edge of every ring. That is enough to be exact.
A coordinate *inside* the area is snapped to whichever vertex makes its journey shortest,
and from that vertex the tree already holds the shortest way to every entry point, so the
route is the one the whole visibility graph would have given -- for a mesh that grows as
entry points times vertices rather than as vertices squared. The ring edges are what make
the perimeter of a plaza and the sides of an obstacle walkable in their own right. An
entry point is where another way connects to the perimeter of the area.

Set `area_emit_visibility_graph = true` in your profile's properties to keep the whole
graph instead. This is a debugging aid, useful for looking at what the mesher saw, and not
how a journey between two arbitrary vertices is served: the extractor stores the visibility
graph beside the mesh, and the engine solves such a journey over that. Emitting it as ways
costs a way per line of sight where storing it costs two integers.

This feature is still EXPERIMENTAL.

## Configuration

### Using the foot profile

The bundled foot profile already carries everything below, switched off. Open
[profiles/foot.lua](../profiles/foot.lua) and set the flag near the top of the file:

```lua
local enable_area_meshing = true
```

Or leave that file alone and extract with
[profiles/foot_area.lua](../profiles/foot_area.lua), which is the same profile with the
flag turned on:

```
osrm-extract -p profiles/foot_area.lua your-data.osm.pbf
```

When meshing is off the extractor writes no `.osrm.openareas` files, and removes any
left behind by an earlier extraction under the same name, so that a dataset built with one
profile cannot carry the areas of another. The profile then behaves exactly as it did
before the feature existed. The rest of this section is for adding meshing to a profile of
your own.

### Simplifying the outline

An OSM plaza is drawn to be looked at rather than routed across. Its outline follows
kerbstones and planting beds at a resolution nothing downstream can use: Ile-de-France has
a pedestrian area with 2739 nodes around 228 by 209 metres, one every 30 cm. Every vertex
is then paid for again on each request, because snapping a coordinate into the area tests
every vertex against every ring edge.

So the extractor drops the vertices that carry no shape, by Visvalingam-Whyatt. It ranks a
vertex by the area of the triangle it makes with its two neighbours and drops the ones
whose triangle is smaller than a threshold, in square metres:

```lua
properties.area_simplify_threshold = 1.0
```

The threshold says directly how much of the plaza a dropped vertex is allowed to add or
remove, so 1.0 means a square metre. Zero switches simplification off and keeps the outline
exactly as drawn. Entry points are never dropped, whatever their triangle: an entrance
simplified away leaves the area unreachable from the way that met it, which is not a loss
of detail but a loss of the graph. No ring is reduced below three vertices, and a ring that
would collapse is left as it is.

The bundled foot profile sets this to 1.0 when meshing is on.

### Adding it to your own profile

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

### What is written, and who reads it

Meshing produces three files beside the usual ones. `.osrm.openareas` holds every meshed
area's rings, flattened, and `.osrm.openareas.ramIndex` and `.fileIndex` hold an r-tree
over their bounding boxes, which is how the engine finds the area a coordinate falls
into.

`.osrm.openareas` also records, for each vertex of each area, the edge-based node
segments standing on it. Snapping a coordinate inside an area offers one candidate per
vertex the coordinate can see, and it needs to know which edges leave each of those
vertices. Asking the r-tree was one nearest-neighbour traversal per visible vertex, which
was most of the cost of a request into a plaza; the extractor knows the answer, so it
writes it down. The segments are stored as the r-tree stores them, and the engine builds
the phantom for the vertex from them the same way a search would have, at the vertex's
own coordinate. At most eight are used per vertex, the budget the search had, since one
edge out of a vertex is enough for the search to reach the rest by turning.

`.osrm.openareas` also carries each area's visibility graph, laid out the same way: for
every vertex, the in-area indices of the vertices it can see, ring neighbours included. A
journey with both ends inside one area is solved over that graph. The mesher computes it
once with its rotational sweep and used to throw it away after pruning it to the mesh,
leaving the engine to rebuild it per request at a cost cubic in the vertex count, which is
what the `GEODESIC_MAX_VERTICES` ceiling was for. With the graph stored the ceiling only
applies to polygons the mesher declined, for which the engine still builds its own.

Those segments name edge-based nodes by id, and `osrm-partition` renumbers the edge-based
nodes for cell locality. It renumbers the stored segments in the same step, alongside the
r-tree leaves and the node data. A partitioned dataset whose `.osrm.openareas` came from
somewhere else is wrong in a way nothing checks, so keep the files of one extraction
together.

The format changed when the segments were added, so data extracted before that has to be
extracted again.

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
<br> A complete example profile is found in the file: [profiles/foot.lua](../profiles/foot.lua),
where every step above is guarded by the `enable_area_meshing` flag.
<br> https://wiki.openstreetmap.org/wiki/Relation:multipolygon
<br> https://wiki.openstreetmap.org/wiki/Key:area
<br> https://wiki.openstreetmap.org/wiki/Tag:highway%3Dpedestrian#Squares_and_plazas
