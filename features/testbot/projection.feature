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
            | from | to | route   | bearing       | distance     |
            | b    | a  | abc,abc | 0->225,225->0 | 1002.9m      |
            | b    | c  | abc,abc | 0->45,45->0   | 1005m +- 3   |
            | a    | d  | abc,abc | 0->45,45->0   | 1002.9m      |
            | d    | a  | abc,abc | 0->225,225->0 | 1002.9m      |
            | c    | d  | abc,abc | 0->225,225->0 | 1005m +- 3   |
            | d    | c  | abc,abc | 0->45,45->0   | 1005m +- 3   |

    Scenario: Projection onto way at high latitudes, no distance
        When I route I should get
            | from | to | route     | distance |
            | d    | b  | abc,abc   | 0m       |
            | b    | d  | abc,abc   | 0m       |


    Scenario: Projection results negative duration
        Given the profile file
        """
        functions = require('testbot')

        function segment_function(profile, segment)
          segment.weight = 5.5
          segment.duration = 2.8
        end

        functions.process_segment = segment_function
        return functions
        """

        Given the node locations
            | node |        lon |        lat |
            |    e | -51.218994 | -30.023866 |
            |    f | -51.218918 | -30.023741 |
            |    1 | -51.219109 | -30.023766 |
            |    2 | -51.219109 | -30.023764 |
            |    3 | -51.219109 | -30.023763 |
            |    4 | -51.219109 | -30.023762 |
            |    5 | -51.219109 | -30.023761 |
            |    6 | -51.219109 | -30.023756 |

        And the ways
            | nodes |
            | ef    |

        When I route I should get
            | waypoints | route | distance | time | weight |
            | 1,4       | ef,ef | 0.4m     | 0.1s |    0.1 |
            | 2,4       | ef,ef | 0.1m     | 0s   |      0 |
            | 3,4       | ef,ef | 0.1m     | 0s   |      0 |
            | 4,4       | ef,ef | 0m       | 0s   |      0 |
            | 5,4       | ef,ef | 0.1m     | 0s   |    0.1 |
            | 6,4       | ef,ef | 0.6m     | 0.1s |    0.2 |
