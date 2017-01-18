@routing @projection @testbot
Feature: Projection to nearest point on road
# Waypoints are projected perpendiculary onto the closest road

    Background:
    # The coordinas below was calculated using http://www.movable-type.co.uk/scripts/latlong.html
    # The nodes are placed as follows, with ab, bc and bd all being 1 km in length each:
    #   |   |   | c |
    #   |   | b |   |   (this is sketch only, real data is in the table below)
    #   | a |   | d |

        Given the profile "testbot.lua"
        Given the node locations
            | node | lat      | lon     |
            | a    | 80.00000 | 0.00000 |
            | b    | 80.00639 | 0.03667 |
            | c    | 80.01278 | 0.07333 |
            | d    | 80.00000 | 0.07333 |

        And the ways
            | nodes |
            | abc   |

    Scenario: Projection onto way at high latitudes, 1km distance
        When I route I should get
            | from | to | route   | bearing   | distance   |
            | b    | a  | abc,abc | 0->225,225->0 | 1000m +- 7 |
            | b    | c  | abc,abc | 0->45,45->0 | 1000m +- 7 |
            | a    | d  | abc,abc | 0->45,45->0 | 1000m +- 7 |
            | d    | a  | abc,abc | 0->225,225->0 | 1000m +- 7 |
            | c    | d  | abc,abc | 0->225,224->0 | 1000m +- 8 |
            | d    | c  | abc,abc | 0->44,45->0  | 1000m +- 8 |

    Scenario: Projection onto way at high latitudes, no distance
        When I route I should get
            | from | to | route     | distance |
            | d    | b  | abc,abc   | 0m  +-5  |
            | b    | d  | abc,abc   | 0m  +-5  |
