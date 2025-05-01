# Areas

This OSRM feature provides routing through areas. The area type is configurable.

## Routing over pedestrian areas

Pedestrian areas in OSM are either closed ways or multipolygon relations. Currently OSRM
routes along the perimeter of a closed way area. It does not route over multipolygon
areas at all.

This feature routes over the inside of the area. It does so by "meshing" the area, ie.
by creating virtual ways between every two entry points of the area. These new ways
follow lines of sight, they never go through obstacles in the area.

This feature is opt-in: To enable it you must define a `process_relation` function in
your profile and return it like this:

```lua
return {
  setup = setup,
  process_way =  process_way,
  process_node = process_node,
  process_turn = process_turn,
  process_relation = process_relation
}
```

You must also keep multipolygon relations, so that you can use the name on the relation
for turn directions. (Remember that the ways in the relation are untagged.) In your
profile's setup function add or edit the `relation_types` sequence to include the type
"multipolygon":

```lua
function setup()
  ...
  return {
    ...
    relation_types = Sequence {
      "multipolygon"
    }
    ...
  }
end
```

### process_relation(profile, relation, relations)

The `process_relation` function is called for every relation in the input file. If you
want a relation to be meshed, call `area_manager:relation(relation)`.

Example of a process_relation function:

```lua
function process_relation(profile, relation, relations)
  type = relation:get_value_by_key('type')
  highway = relation:get_value_by_key('highway')
  if type == 'multipolygon' and highway == 'pedestrian' then
    -- register the relation
    area_manager:relation(relation)
  end
end
```

### process_way(profile, way, result, relations)

The `process_way` function is called for every way in the input file. If you want a
closed way to be meshed, call `area_manager:way(way)`. (Note that open ways cannot be
meshed and will be ignored.)

Multipolygons need some support too. Since the member ways of a multipolygon relation
are as a rule untagged, you have to copy at least the defining tag (and maybe the name)
from the relation to the way. OSRM discards untagged ways.

Example of a process_way function:

```lua
function process_way(profile, way, result, relations)
  ...
  if way:get_value_by_key('highway') == 'pedestrian' then
    -- register the way
    area_manager:way(way)
  end

  for _, rel_id in pairs(area_manager:get_relations(way)) do
    -- if this way is a member of a registered relation
    -- we have to set at least one defining tag
    local rel = relations:relation(rel_id)
    data.highway = rel:get_value_by_key('highway')
  end
  ...
end
```

### area_manager

A global user type.

#### area_manager:relation(relation)
Call this function inside `process_relation()` to register a relation for meshing. The
relation must be a multipolygon relation.

Argument | Type        | Notes
---------|-------------|-----------------------------------------------------
relation | OSMRelation | The same relation as passed into `process_relation`.

#### area_manager:way(way)
Call this function inside `process_way()` to register a way for meshing. The way must be
closed.

Argument | Type     | Notes
---------|----------|-------------------------------------------
way      | OSMWay   | The same way as passed into `process_way`.

#### area_manager:get_relations(way)
Call this function inside `process_way()`.  If this way is a member of a relation that
was registered for meshing, that relation will be returned. Since the member ways of a
multipolygon relation are as a rule untagged, you have to copy at least the defining tag
(and maybe the name) from the relation to the way. OSRM discards untagged ways.

Argument | Type     | Notes
---------|----------|-------------------------------------------
way      | OSMWay   | The same way as passed into `process_way`.

Usage example:

```lua
for _, rel_id in pairs(area_manager:get_relations(way)) do
  local rel = relations:relation(rel_id)
  data.highway = rel:get_value_by_key('highway')
  WayHandlers.names(profile, rel, result, data)
end
```
