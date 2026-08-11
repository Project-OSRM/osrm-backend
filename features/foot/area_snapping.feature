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
            | x    | f  | bf      | B,B   |
            | x    | g  | cg      | D,D   |
            | e    | x  | ea      | A,A   |

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
            | p    | i  | ni      | E,E   |
            | e    | p  | ea      | A,A   |

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
            | p    | f  | ubf     | ,B    |
            | p    | g  | xcg     | ,D    |
            | e    | q  | eav     | A,,   |

    # Both ends inside one area: they route by way of a shared vertex rather than
    # straight across, because neither endpoint inserts an edge to the other.  Pinned so
    # that closing that gap is a visible change -- see plans/open-area-snapping.md.
    Scenario: Foot - Start and end inside the same plaza
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
            | p    | q  | uv      |

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
            | q    | e  | xae     | ,A,A  |
            | e    | q  | eax     | A,,   |

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

        # the sight line from one obstacle's corner to the other's runs over both of
        # them, which is shorter than going round by the plaza's edge.  Asserted as
        # geometry: the node ids at the ends of a leg name the way the phantom sits on
        # rather than the way the route took, and CH and MLD do not always pick the same
        # one (see #7683).
        When I route I should get
            | from | to | geometry            |
            | p    | s  | efbEsnbE?aM         |
            | p    | g  | o`bEsnbExAsR?yA     |

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
            | p    | t  | xnt     | ,T,T      |
            | s    | p  | smx     | S,,       |
            | s    | t  | smnt    | S,,T,T    |
