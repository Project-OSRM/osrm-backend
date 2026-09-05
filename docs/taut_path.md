# Pulling a path taut and rounding its corners

This describes how a walking path across an open area (a plaza, a square, a
pedestrian zone) is turned into a path that is short and looks smooth on a map.
The code is in `src/engine/area_fillet.cpp`. The two entry points are
`pull_taut()` and `round_corners()`.

## The setting

An open area is a polygon. Its outer ring is the edge of the area. Inside it
there can be obstacles, each a polygon of its own: a fountain, a building, a
planter, a tree pit. A person can walk anywhere inside the outer ring that is
not inside an obstacle. Walking along the edge of an obstacle, or along the
outer ring, is allowed.

A path is a list of points. The first and the last point are fixed; they are
where the person starts and ends. Every point in between can be moved or
removed. A straight segment between two points is *legal* if no part of it is
inside an obstacle or outside the area. A path is legal if all its segments
are.

The input path can come from anywhere: a shortest-path search, a grid search,
a hand-drawn line, a recorded track. It may have many points and many small
bends. The only requirement is that it is legal.

The output is a path with the same two end points that is:

1. as short as possible without changing which side of a large obstacle it
   passes,
2. made of straight lines and circular arcs only, and
3. held a chosen distance (the *margin*) away from obstacles wherever there is
   room.

The work is done in two stages. The first stage makes the path short. The
second stage makes it smooth.

## Stage 1: pulling the path taut

Think of the path as a string laid through the area, with its two ends pinned.
Pulling the string tight gives the shortest path that stays on the same side of
every obstacle. That is what this stage computes. It runs in rounds until
nothing changes.

### Step 1a: remove points that are not needed

Take three consecutive points A, B, C. If the straight segment from A to C is
legal, B may not be needed. But that is not enough on its own. The segment
A to C together with the two segments A to B and B to C form a triangle, and
something may lie inside that triangle. If an obstacle lies inside it, then
going from A directly to C passes that obstacle on the other side. That would
change the route.

So B is removed only if the segment A to C is legal *and* the triangle holds no
obstacle that the path is not allowed to pass on the other side. Which
obstacles those are is explained below.

The same idea is applied to longer stretches. One segment may replace up to six
consecutive points at once, and the check is then made on the polygon formed by
the removed stretch and the new segment, instead of a triangle. This matters
for small obstacles: a path that goes round a planter touches two or three of
its corners, and removing one of those corners at a time never works, because
the shortcut across a single corner cuts through the planter itself.

### Which obstacles may be passed on the other side

An obstacle that lies completely inside the region formed by the old stretch
and the new segment is being *hopped*: the path moves from one side of it to
the other. This is allowed only for small obstacles. The limit is 8 metres
across the obstacle's bounding box. That size separates street furniture
(planters, tree pits, benches, bollards) from things a person would consider a
real detour (kiosks, fountains, buildings). The outer ring of the area is never
hopped.

An obstacle that lies only partly inside the region blocks the change. Since
the old stretch and the new segment are both legal, the only way an obstacle
can reach into the region is by being the thing the old stretch went round, so
the new segment would pass it on the other side.

Checking whether an obstacle is inside the region is done by testing the
obstacle's corner points against the region's polygon. This is enough because
a legal segment cannot cross an obstacle's edge, so an obstacle cannot enter
the region without one of its corners being inside it.

A point that lies exactly on the region's boundary, which is the case for the
obstacle corners the path stands on, is counted as neither inside nor outside.

### Step 1b: pull the remaining points onto the obstacles

After step 1a, every remaining point is needed, but it is still where the input
put it. That may be some distance away from the obstacle it goes round. A
string pulled tight would rest on the obstacle's corner.

So each remaining point is moved. First it is moved towards the straight line
between its two neighbours, in steps, as far as the move is *clean*. Then it is
offered every corner of every obstacle as a place to stand, and takes the one
that makes the path shortest, if that move is clean too.

A move of a point from one place to another is clean if the two new segments
to its neighbours are legal, the segment from the old place to the new place
is legal, and the two triangles swept by the move contain no obstacle that may
not be hopped. This is the same check as in step 1a, applied to a move instead
of a removal, and it keeps the path on the same side of every large obstacle.

A point that ends up on the straight line between its neighbours is removed in
the next round.

### Step 1c: ask the route planner when a shortcut needs new corners

Steps 1a and 1b can only remove points or move them. Sometimes the shorter way
past a small obstacle needs points the path does not have. A typical case is a
planter standing where two corridors meet: the path goes round the outside of
the planter, and the shorter way on the inside has to turn on the corners of
the block across the corridor.

For this case the code can be given a planner: a function that returns the
shortest legal path between two points of the area. The engine uses the same
shortest-path search it uses for routes across the area. When no shortcut
segment is legal for a stretch, and that stretch encloses an obstacle small
enough to hop, the planner is asked for the shortest way between the two ends
of the stretch. Its answer replaces the stretch if it is shorter, legal, and
the region between the old stretch and the new one contains nothing that may
not be hopped. The last condition means the planner cannot move the path to
the other side of a building even when that would be shorter.

Without a planner, steps 1a and 1b still run; only this case is left as it is.

### What stage 1 produces

A path with few points. Each point that is not an end point lies on a corner
of an obstacle or of the outer ring, and it is there because the straight line
across it is not legal. The path passes every large obstacle on the same side
as the input did. Small obstacles are passed on whichever side is shorter.

## Stage 2: rounding the corners

The path from stage 1 is straight segments meeting at sharp corners, and each
corner touches the obstacle it turns around. This stage moves each corner away
from its obstacle and replaces it with a circular arc. Straight segments stay
straight. The result contains only straight lines and arcs, so it cannot
zigzag.

### Step 2a: move each corner away from its obstacle

At a corner, the path turns around an obstacle that lies on the inside of the
turn. The corner is moved outward, along the line that bisects the turn.

The largest useful move is `margin / cos(turn angle / 2)`. At that distance
both segments meeting at the corner run exactly `margin` away from the edges
they were touching. The corner is not always moved that far. The bisector is
sampled at eight points up to that distance. The first sample with at least
`margin` of clear space around it is taken. If no sample has that much, the
sample with the most clear space is taken. If a sample is outside the area or
inside an obstacle, the search stops there.

This means: in the open, a corner moves the full distance; in a passage
narrower than twice the margin, it moves to the middle of the passage; and it
never moves through a wall. The two end points of the path are never moved.

### Step 2b: replace each corner with an arc

At each moved corner, an arc is drawn that is tangent to the segment coming in
and the segment going out. The radius is the margin, except that it is reduced
in three cases:

* A segment between two corners can give at most half its length to each of
  the two arcs, so that the arcs do not overlap.
* A segment from an end point can give at most 90% of its length, so a short
  straight piece always remains at the end point.
* The radius is limited so that the arc's mid point does not come back to the
  obstacle's corner. The mid point of the arc lies `radius * (1 / cos(turn / 2)
  - 1)` closer to the corner than the moved point, and that distance may not
  exceed how far the corner was moved.

The arc is written out as points, one every eighth of the margin along the arc,
which is about seven degrees per point.

### Step 2c: check the result and back off where it fails

Every segment of the result is checked for legality. Three things can make a
corner fail:

* one of its arc's segments, or the straight segment next to it, is not legal;
* the straight segment between two corners changed its direction by more than
  60 degrees, or lost more than 80% of its length, because the two corners
  were pushed apart (this makes the two arcs meet in a sharp cusp);
* the turn at the corner became more than 15 degrees sharper than it was
  before the corner was moved (this happens next to an end point, which does
  not move while the corner beside it does, and the arc through a very sharp
  turn is a hook).

A failing corner has its move distance and its radius halved, and the whole
path is built again. This repeats until nothing fails. A corner that has been
halved down to nothing stays where stage 1 put it, with no arc. Such a corner
is always legal, so the process always ends.

### Step 2d: reduce the number of points

The result is simplified with the Douglas-Peucker method to 0.1 metres: points
are removed as long as the path never moves more than 0.1 metres from where it
was. A straight run becomes two points, and an arc becomes a handful. The two
end points are returned exactly as they were given.

## What the result guarantees

* Every segment of the result is legal.
* The result passes every obstacle larger than 8 metres on the same side as the
  input.
* The result consists of straight segments and circular arcs only.
* The same input always gives the same output. Every choice is made by
  comparing numbers computed from the input; nothing depends on timing or on
  the order in which things are stored.
* The cost is a few microseconds for a path across a plaza.

## What it does not do

* It does not move a straight segment away from a wall it runs along without
  turning. Only corners are moved.
* It does not make a corner sharper than the input had it, but it also does
  not guarantee that every corner becomes an arc. Where there is no room, the
  corner stays sharp rather than cutting into an obstacle.
* It does not choose which side of a large obstacle to pass. That is decided
  by whoever produced the input path.

## The parameters

| Name | Value | Meaning |
|---|---|---|
| margin | set by the profile, in metres | how far the path is held from obstacles, and the radius of the arcs |
| hop size | 8 m | an obstacle up to this size across may be passed on either side |
| points removed at once | 6 | how many consecutive points one shortcut segment may replace |
| thinning tolerance | 0.1 m | how far a point may be from the simplified path |
| arc step | margin / 8 | distance between the points written along an arc |
