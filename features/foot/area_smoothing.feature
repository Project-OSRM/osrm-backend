@routing @foot @area
Feature: Foot - Smoothing a route drawn across a plaza

    # The shortest way across a plaza bends exactly on the corners it passes.  With a
    # comfort margin set the leg is handed to the elastic band (engine/area_band.hpp),
    # which holds it that far off the geometry and rounds the corners it turns.
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
    # 10 m margin it leaves p on a curve, passes each corner 10 m off it, and runs the
    # length of the obstacle 10 m off its edge: 28 points against the taut path's 4, and
    # 24 m longer, which is what holding the margin costs.
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
            | p    | q  | kcbEembEWKKCGAGAKCKAKAEAEAAAAAAEAC?C?uA?eG?G@E@A@CDCB?LAJANERE\K             | 335m +-2 |
