@routing @foot @area
Feature: Foot - Smoothing a route drawn across a plaza

    # The shortest way across a plaza bends exactly on the corners it passes.  With a
    # margin set the leg is redrawn as straight lines and arcs (engine/area_fillet.hpp),
    # each corner moved that far off the geometry and replaced by a tangent arc.
    #
    # Off by default, and every scenario in area_snapping.feature runs with it off; this
    # file is the one place it is on, so the diff that changes the drawn shape of a plaza
    # route is this file and nothing else.  What is asserted is the geometry, since the
    # shape is the whole point, and the distance beside it is the drawn line's own.

    Background:
        Given the profile file "foot_area" initialized with
            """
            profile.properties.area_smoothing_margin = 10
            """
        Given a grid size of 50 meters
        Given the query options
            | overview | full |

    # The same plaza and obstacle as area_snapping.feature's "with an obstacle in the
    # way", where the taut route is 312 m: up to the obstacle's corner, 200 m along its
    # edge, and down from the other corner, turning 63 degrees at each.  Smoothed at a
    # 10 m margin it is straight lines and arcs: a straight run from p, a 10 m arc past
    # each corner, and a straight run about 10 m off the obstacle's edge between them, 15
    # points against the taut path's 4 and 19 m longer, which is what the margin costs.
    Scenario: Foot - The corners of an obstacle are rounded and the margin is held
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
            | from | to | geometry                                                              | distance |
            | p    | q  | kcbEembE}A_@CAAAACAA?AAC?aJ@C?C@CBA@A~A_@ | 331m +-2 |
