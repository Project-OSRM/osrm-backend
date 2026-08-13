@routing @foot @area @matrix
Feature: Foot - Tables and trips across a pedestrian area

    # A pair of coordinates on one plaza is a journey the mesh answers badly: it holds
    # shortest paths *out* of an area, and this journey never leaves.  /route works the
    # answer out from the polygon instead.  A table is that question asked many times over
    # and a trip picks its tour from a table before drawing it, so both have to give the
    # same answer as /route or the three services contradict each other.
    #
    # Only pairs with *both* ends on the plaza are asserted here.  A pair with one end on
    # it is a separate matter and is not yet right -- see plans/area-table-trip.md.

    Background:
        Given the profile "foot_area"
        Given a grid size of 50 meters

    Scenario: Foot - Distance table between two points on one plaza
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

        # m and n can see each other, so the cell between them is the straight line --
        # 100 m, the same as /route reports, and not a detour by way of a corner
        When I request a travel distance matrix I should get
            |   | m     | n     |
            | m | 0     | 100.1 |
            | n | 100.1 | 0     |

        # and the same journey in seconds, at the area's walking speed
        When I request a travel time matrix I should get
            |   | m    | n    |
            | m | 0    | 71.5 |
            | n | 71.5 | 0    |

    Scenario: Foot - Distance table across an obstacle
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

        # p and q are on opposite sides of the obstacle, so the cell between them is the
        # way round it -- the same 312 m /route reports, not the 300 m straight line
        When I request a travel distance matrix I should get
            |   | p     | q     |
            | p | 0     | 311.8 |
            | q | 311.8 | 0     |

    Scenario: Foot - Trip round three points on one plaza
        Given the node map
            """
            e-a-----------b-f
              | u-------v |
              |p|       |q|
              | x-------w |
              |     s     |
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

        # every leg stays on the plaza, so the tour is chosen from geodesics and drawn as
        # geodesics -- the same lines /route would give between the same pairs
        When I plan a trip I should get
            | waypoints | trips | distance |
            | p,q,s     | psq   | 647m +-6 |

    # A pair with one end off the plaza.  Walking is symmetric, so the matrix has to be:
    # an asymmetric cell for a foot profile is the bug announcing itself.  Both cells are
    # 161.7 m, which is 50 m along way A and then 111.8 m straight across the plaza, and
    # is what /route reports in both directions.
    Scenario: Foot - A pair with only one end on the plaza
        Given the node map
            """
            e-a-------b-f
              |       |
              | m     |
            h-d-------c-g
            """

        And the ways
            | nodes | highway    | area | name  |
            | abcda | pedestrian | yes  | Plaza |
            | ea    | pedestrian |      | A     |
            | bf    | pedestrian |      | B     |
            | hd    | pedestrian |      | C     |
            | cg    | pedestrian |      | D     |

        When I route I should get
            | from | to | distance | time   |
            | m    | e  | 161.7m   | 115.8s |
            | e    | m  | 161.7m   | 115.8s |

        When I request a travel distance matrix I should get
            |   | m     | e     |
            | m | 0     | 161.7 |
            | e | 161.7 | 0     |

        When I request a travel time matrix I should get
            |   | m     | e     |
            | m | 0     | 115.8 |
            | e | 115.8 | 0     |
