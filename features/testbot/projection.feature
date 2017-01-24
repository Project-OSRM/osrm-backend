@routing @projection @testbot
Feature: Projection to nearest point on road
# Waypoints are projected perpendiculary onto the closest road

    Background:
    # The coordinas below was calculated using http://www.movable-type.co.uk/scripts/latlong.html
    # The nodes are placed as follows, with ab, bc and bd all being 1 km in length each:
    #   |   |   | c |
    #   |   | b |   |   (this is sketch only, real data is in the table below)
    #   | a |   | d |

        Given the profile "testbot"
        Given the node locations
            | node | lat       | lon       |
            | a    | 80.000000 | 0.0000000 |
            | b    | 80.006350 | 0.0366666 |
            | c    | 80.012730 | 0.0733333 |
            | d    | 80.000000 | 0.0733333 |

        And the ways
            | nodes |
            | abc   |

    Scenario: Projection onto way at high latitudes, 1km distance
        When I route I should get
            | from | to | route   | bearing       | distance   |
            | b    | a  | abc,abc | 0->225,225->0 | 1000m      |
            | b    | c  | abc,abc | 0->45,45->0   | 1000m +- 3 |
            | a    | d  | abc,abc | 0->45,45->0   | 1000m      |
            | d    | a  | abc,abc | 0->225,225->0 | 1000m      |
            | c    | d  | abc,abc | 0->225,225->0 | 1000m +- 3 |
            | d    | c  | abc,abc | 0->45,45->0   | 1000m +- 3 |

    Scenario: Projection onto way at high latitudes, no distance
        When I route I should get
            | from | to | route     | distance |
            | d    | b  | abc,abc   | 0m       |
            | b    | d  | abc,abc   | 0m       |
