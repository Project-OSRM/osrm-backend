@routing @foot @area
Feature: Foot - Snapping inside a pedestrian area

    # A coordinate that falls inside a meshed area does not snap to the nearest meshed
    # line.  It snaps to the vertex of the area that makes the whole journey shortest,
    # walking there in a straight line -- the route the visibility graph would have given
    # if that coordinate had been part of it when it was built.
    #
    # These scenarios mirror the fixtures drawn by scripts/debug/area_snapping_report.py,
    # one per shape, covering a route that starts inside an area, one that ends inside
    # one, and one that crosses it.  The interior points are nodes that belong to no way,
    # so the only way to reach them is by snapping into the area.
    #
    # Several of them assert a route through a corner of an *obstacle*.  Those corners
    # only carry edges when the whole visibility graph is emitted; with the entry-point
    # mesh they are dead ends and the routes detour to the plaza's own corners instead.
    # So these double as a guard on that.

    Background:
        Given the profile "foot_area"
        Given a grid size of 50 meters
        Given the query options
            | annotations | nodes |

    Scenario: Foot - Start or end inside a plaza
        Given the node map
            """
            e-a-------b-f
              |       |
              |   x   |
            h-d-------c-g
            """

        And the ways
            | nodes | highway    | area | name  |
            | abcda | pedestrian | yes  | Plaza |
            | ea    | pedestrian |      | A     |
            | bf    | pedestrian |      | B     |
            | hd    | pedestrian |      | C     |
            | cg    | pedestrian |      | D     |

        # x leaves by whichever corner is on the way to where it is going, and is
        # arrived at the same way
        When I route I should get
            | from | to | a:nodes | route |
            | x    | f  | bbf     | B,B   |
            | x    | g  | ccg     | D,D   |
            | e    | x  | aae     | A,A   |

    Scenario: Foot - Plaza entered by the middle of an edge
        Given the node map
            """
            e-a---m---b-f
              |       |
              |   p   |
            h-d---n---c-g
                  |
                  i
            """

        And the ways
            | nodes   | highway    | area | name  |
            | ambcnda | pedestrian | yes  | Plaza |
            | ea      | pedestrian |      | A     |
            | bf      | pedestrian |      | B     |
            | hd      | pedestrian |      | C     |
            | cg      | pedestrian |      | D     |
            | ni      | pedestrian |      | E     |

        # n is nearer than any corner, and is where the route to i should set off from
        When I route I should get
            | from | to | a:nodes | route |
            | p    | i  | nni     | E,E   |
            | e    | p  | aae     | A,A   |

    Scenario: Foot - Route round an obstacle from inside the plaza
        Given the node map
            """
            e-a-----------b-f
              | u-------v |
              |p|       |q|
              | x-------w |
            h-d-----------c-g
            """

        And the ways
            | nodes | highway    | name |
            | abcda | (nil)      |      |
            | uvwxu | (nil)      |      |
            | ea    | pedestrian | A    |
            | bf    | pedestrian | B    |
            | hd    | pedestrian | C    |
            | cg    | pedestrian | D    |

        And the relations
            | type         | highway    | way:outer | way:inner |
            | multipolygon | pedestrian | abcda     | uvwxu     |

        # p is wedged between the plaza's edge and the obstacle, so every route out of it
        # turns a corner of the obstacle -- u going north, x going south
        When I route I should get
            | from | to | a:nodes | route |
            | p    | f  | uubf    | ,B    |
            | p    | g  | xxcg    | ,D    |

        # The node ending the list names the target phantom's own segment, and a leg
        # that ends at a free point keeps whichever phantom the search settled on --
        # which differs between CH and MLD.  Assert the distance, which does not.
        When I route I should get
            | from | to | distance   |
            | e    | q  | 361m +-6   |

    # Both ends on one plaza is the case the mesh cannot answer: it holds shortest paths
    # *out* of an area, and this journey never leaves.  The route is worked out from the
    # polygon when asked instead -- see engine/area_route.hpp -- so it bends only where it
    # has to, and starts and ends exactly where it was asked to.
    Scenario: Foot - Across a plaza from one point on it to another
        Given the node map
            """
            e-a-------b-f
              |       |
              | m   n |
            h-d-------c-g
            """

        And the ways
            | nodes | highway    | area | name  |
            | abcda | pedestrian | yes  | Plaza |
            | ea    | pedestrian |      | A     |
            | bf    | pedestrian |      | B     |
            | hd    | pedestrian |      | C     |
            | cg    | pedestrian |      | D     |

        # m and n can see each other, so the answer is the straight line between them --
        # 100 m, not a detour by way of a corner
        When I route I should get
            | from | to | distance  | route       |
            | m    | n  | 100m +-3  | Plaza,Plaza |
            | n    | m  | 100m +-3  | Plaza,Plaza |

    # With something in the way it has to bend, and bends at a corner of the obstacle.
    Scenario: Foot - Across a plaza with an obstacle in the way
        Given the node map
            """
            e-a-----------b-f
              | u-------v |
              |p|       |q|
              | x-------w |
            h-d-----------c-g
            """

        And the ways
            | nodes | highway    | name |
            | abcda | (nil)      |      |
            | uvwxu | (nil)      |      |
            | ea    | pedestrian | A    |
            | bf    | pedestrian | B    |
            | hd    | pedestrian | C    |
            | cg    | pedestrian | D    |

        And the relations
            | type         | highway    | way:outer | way:inner |
            | multipolygon | pedestrian | abcda     | uvwxu     |

        # straight across would be 300 m and runs through the obstacle; over the top of
        # it is 312 m.  The distance is what is asserted, not the node list: a leg between
        # two free points is written out by the engine, and the nodes naming its two ends
        # come from whichever phantom the search happened to keep, which differs between
        # CH and MLD.  The bends in between are the part that matters, and they show up in
        # the distance.
        When I route I should get
            | from | to | distance  |
            | p    | q  | 312m +-5  |

    # A journey with only one end on the plaza is not this case, and is left to the mesh.
    Scenario: Foot - One end on the plaza is routed by the mesh
        Given the node map
            """
            e-a-----------b-f
              | u-------v |
              |p|       |q|
              | x-------w |
            h-d-----------c-g
            """

        And the ways
            | nodes | highway    | name |
            | abcda | (nil)      |      |
            | uvwxu | (nil)      |      |
            | ea    | pedestrian | A    |
            | bf    | pedestrian | B    |
            | hd    | pedestrian | C    |
            | cg    | pedestrian | D    |

        And the relations
            | type         | highway    | way:outer | way:inner |
            | multipolygon | pedestrian | abcda     | uvwxu     |

        When I route I should get
            | from | to | a:nodes |
            | p    | f  | uubf    |

        # The node ending the list names the target phantom's own segment, and a leg
        # that ends at a free point keeps whichever phantom the search settled on --
        # which differs between CH and MLD.  Assert the distance, which does not.
        When I route I should get
            | from | to | distance   |
            | e    | q  | 361m +-6   |

    # A via point on the plaza makes two legs, and each is solved the same way.
    Scenario: Foot - Via a point on the plaza
        Given the node map
            """
            e-a-------b-f
              |       |
              | m k n |
            h-d-------c-g
            """

        And the ways
            | nodes | highway    | area | name  |
            | abcda | pedestrian | yes  | Plaza |
            | ea    | pedestrian |      | A     |
            | bf    | pedestrian |      | B     |
            | hd    | pedestrian |      | C     |
            | cg    | pedestrian |      | D     |

        # m, k and n are in a line, so going by way of k costs nothing over going direct
        When I route I should get
            | waypoints | distance  |
            | m,k,n     | 100m +-3  |

    Scenario: Foot - Obstacle against one side of the plaza
        Given the node map
            """
            e-a-----------b-f
              | u-------v |
              | |       | |
              | x-------w |
              |          q|
            h-d-----------c-g
            """

        And the ways
            | nodes | highway    | name |
            | abcda | (nil)      |      |
            | uvwxu | (nil)      |      |
            | ea    | pedestrian | A    |
            | bf    | pedestrian | B    |
            | hd    | pedestrian | C    |
            | cg    | pedestrian | D    |

        And the relations
            | type         | highway    | way:outer | way:inner |
            | multipolygon | pedestrian | abcda     | uvwxu     |

        # the obstacle hides the whole north-west of the plaza from q, so the way out
        # runs under it, by way of its south-west corner
        When I route I should get
            | from | to | a:nodes | route |
            | q    | e  | xxae    | ,A,A  |

        # The node ending the list names the target phantom's own segment, and a leg
        # that ends at a free point keeps whichever phantom the search settled on --
        # which differs between CH and MLD.  Assert the distance, which does not.
        When I route I should get
            | from | to | distance   |
            | e    | q  | 439m +-6   |

    Scenario: Foot - Two obstacles with a gap between them
        Given the node map
            """
            e-a---------------b-f
              | u-v     i-j   |
              |p| |     | |  s|
              | x-w     l-k   |
            h-d---------------c-g
            """

        And the ways
            | nodes | highway    | name |
            | abcda | (nil)      |      |
            | uvwxu | (nil)      |      |
            | ijkli | (nil)      |      |
            | ea    | pedestrian | A    |
            | bf    | pedestrian | B    |
            | hd    | pedestrian | C    |
            | cg    | pedestrian | D    |

        And the relations
            | type         | highway    | way:outer | way:inner   |
            | multipolygon | pedestrian | abcda     | uvwxu,ijkli |

        # p to g leaves the area, so the mesh answers it.  p to s never leaves, so the
        # geodesic does -- over the top of both obstacles rather than round them.
        When I route I should get
            | from | to | distance   |
            | p    | s  | 396m +-6   |

        # p to g leaves the area, so the mesh answers it and the node list is stable
        When I route I should get
            | from | to | a:nodes | distance   |
            | p    | g  | xxcg    | 460m +-6   |

    Scenario: Foot - Plaza with two entrances on the same side
        Given the node map
            """
            a-------------b
            | u-------v   |
            |p|       |   |
            | x-------w   |
            d---m-----n---c
                |     |
                s     t
            """

        And the ways
            | nodes   | highway    | name |
            | abcnmda | (nil)      |      |
            | uvwxu   | (nil)      |      |
            | sm      | pedestrian | S    |
            | tn      | pedestrian | T    |

        And the relations
            | type         | highway    | way:outer | way:inner |
            | multipolygon | pedestrian | abcnmda   | uvwxu     |

        # Nothing joins the north of this plaza to anything, so the entry-point mesh is a
        # single chord along the bottom and p has no graph anywhere near it.  Reaching an
        # entrance from behind the obstacle is the case the whole visibility graph exists
        # for.
        When I route I should get
            | from | to | a:nodes | route     |
            | p    | t  | xxnt    | ,T,T      |
            | s    | t  | smnt    | S,,T,T    |

        # The node ending the list names the target phantom's own segment, and a leg
        # that ends at a free point keeps whichever phantom the search settled on --
        # which differs between CH and MLD.  Assert the distance, which does not.
        When I route I should get
            | from | to | distance   |
            | s    | p  | 227m +-6   |
