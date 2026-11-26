@routing @testbot @matrix
Feature: Table searches with equal rounded weights at an intersection

    Background:
        Given the profile "testbot"
        And the node locations
            | node | lon       | lat    |
            | a    | 28.909    | 41     |
            | b    | 28.91     | 41     |
            | c    | 28.911    | 41     |
            | d    | 28.91     | 41.001 |
            | e    | 28.911    | 41.001 |
            | 1    | 28.910001 | 41     |
            | 2    | 28.91     | 41     |
            | 3    | 28.911    | 41     |

    # b offers several snapping candidates. Travelling from 1 back to b must
    # use the loop; a candidate on the outgoing road cannot be used backwards.
    Scenario: A junction keeps its legal alternatives when a zero-cost pairing is backwards
        Given the ways
            | nodes | oneway |
            | ab    | no     |
            | bc    | yes    |
            | ce    | yes    |
            | ed    | yes    |
            | db    | yes    |

        When I request a travel distance matrix I should get
            |   | b        | 1   |
            | b | 0        | 0.1 |
            | 1 | 390 +- 2 | 0   |
        When I request a travel distance matrix I should get
            |   | b        | 1 |
            | 1 | 390 +- 2 | 0 |
        When I request a travel distance matrix I should get
            |   | b        |
            | b | 0        |
            | 1 | 390 +- 2 |
        When I request a travel distance matrix I should get
            |   | b        | 1   |
            | b | 0        | 0.1 |
            | 1 | 390 +- 2 | 0   |
            | 2 | 0        | 0.1 |

    Scenario: Unrelated junction candidates do not make reverse one-way travel reachable
        Given the ways
            | nodes | oneway |
            | ab    | no     |
            | bc    | yes    |
            | ce    | yes    |

        When I request a travel distance matrix I should get
            |   | b | 1   |
            | b | 0 | 0.1 |
            | 1 |   | 0   |
        When I request a travel distance matrix I should get
            |   | b | 1 |
            | 1 |   | 0 |
        When I request a travel distance matrix I should get
            |   | b |
            | b | 0 |
            | 1 |   |
        When I request a travel distance matrix I should get
            |   | b | 1   |
            | b | 0 | 0.1 |
            | 1 |   | 0   |
            | 2 | 0 | 0.1 |

    # Bearings select the segments on either side of c, which is an interior
    # geometry vertex of this way. They are two representations of one position.
    Scenario: Adjacent snapped segments at the same vertex need no detour
        Given the ways
            | nodes | oneway |
            | abced | yes    |
        And the query options
            | bearings | 0,10;90,10 |

        When I request a travel distance matrix I should get
            |   | c | 3 |
            | c | 0 | 0 |
            | 3 | 0 | 0 |
        When I request a travel time matrix I should get
            |   | c | 3 |
            | c | 0 | 0 |
            | 3 | 0 | 0 |

        # Non-square requests list destination coordinates before source coordinates.
        Given the query options
            | bearings | 0,10;90,10;0,10 |
        When I request a travel distance matrix I should get
            |   | c | 3 |
            | c | 0 | 0 |
        When I request a travel time matrix I should get
            |   | c | 3 |
            | c | 0 | 0 |

        Given the query options
            | bearings | 90,10;0,10;90,10 |
        When I request a travel distance matrix I should get
            |   | 3 |
            | c | 0 |
            | 3 | 0 |
        When I request a travel time matrix I should get
            |   | 3 |
            | c | 0 |
            | 3 | 0 |

    # 7 and 8 snap to xy about 4 cm apart. Both snapped coordinates round to the
    # same value. The speed file makes the weight and the duration of xy round
    # differently. Way zv only adds a turn: without a turn, osrm-contract and
    # osrm-customize fail when they apply the speed file.
    Scenario Outline: Points that snap to the same rounded coordinate keep their order
        Given the node locations
            | node | lon      | lat      | id   |
            | w    | -0.001   | -0.0004  | 1001 |
            | x    | 0        | 0        | 1002 |
            | y    | 0.0001   | 0.00004  | 1003 |
            | z    | 0.001    | 0.0004   | 1004 |
            | v    | 0.001    | 0.0014   |      |
            | 7    | 0.000014 | 0.000037 |      |
            | 8    | 0.000014 | 0.000038 |      |
            | 9    | 0.000014 | 0.000037 |      |
        And the ways
            | nodes | oneway |
            | wxyz  | yes    |
            | zv    | yes    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1002,1003,1,12
        """
        And the query options
            | scale_factor | <scale> |

        When I request a travel time matrix I should get
            |   | 7 | 8          |
            | 7 | 0 | <duration> |
            | 8 |   | 0          |
        When I request a travel time matrix I should get
            |   | 7 | 8          |
            | 7 | 0 | <duration> |
        When I request a travel time matrix I should get
            |   | 7 |
            | 7 | 0 |
            | 8 |   |
        When I request a travel time matrix I should get
            |   | 7 | 8          |
            | 7 | 0 | <duration> |
            | 8 |   | 0          |
            | 9 | 0 | <duration> |
        When I request a travel distance matrix I should get
            |   | 7 | 8 |
            | 7 | 0 | 0 |
            | 8 |   | 0 |

        Examples:
            | scale | duration |
            | 1     | 0.1      |
            | 1.6   | 0.2      |

    # The same points on a two-way road. From 8 to 7, the reverse direction gives
    # a legal path. The forward direction gives a backwards path with less weight.
    # The table must use the legal path.
    Scenario Outline: A backwards path on a two-way road does not replace the legal path
        Given the node locations
            | node | lon      | lat      | id   |
            | w    | -0.001   | -0.0004  | 1001 |
            | x    | 0        | 0        | 1002 |
            | y    | 0.0001   | 0.00004  | 1003 |
            | z    | 0.001    | 0.0004   | 1004 |
            | 7    | 0.000014 | 0.000037 |      |
            | 8    | 0.000014 | 0.000038 |      |
            | 9    | 0.000014 | 0.000037 |      |
        And the ways
            | nodes |
            | wxyz  |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1002,1003,1,12
        """
        And the query options
            | scale_factor | <scale> |

        When I request a travel time matrix I should get
            |   | 7          | 8          |
            | 7 | 0          | <duration> |
            | 8 | <duration> | 0          |
        When I request a travel time matrix I should get
            |   | 7          | 8 |
            | 8 | <duration> | 0 |
        When I request a travel time matrix I should get
            |   | 7          |
            | 7 | 0          |
            | 8 | <duration> |
        When I request a travel time matrix I should get
            |   | 7          | 8          |
            | 7 | 0          | <duration> |
            | 8 | <duration> | 0          |
            | 9 | 0          | <duration> |

        Examples:
            | scale | duration |
            | 1     | 0.1      |
            | 1.6   | 0.2      |

    # The way uses oneway=-1, so traffic moves against the drawing direction.
    # Thus 7 is in front of 8, and there is no route from 7 to 8. Way wv only
    # adds a turn.
    Scenario Outline: Points that snap to the same rounded coordinate keep their order on a reversed one-way road
        Given the node locations
            | node | lon      | lat      | id   |
            | w    | -0.001   | -0.0004  | 1001 |
            | x    | 0        | 0        | 1002 |
            | y    | 0.0001   | 0.00004  | 1003 |
            | z    | 0.001    | 0.0004   | 1004 |
            | v    | -0.001   | -0.0014  |      |
            | 7    | 0.000014 | 0.000037 |      |
            | 8    | 0.000014 | 0.000038 |      |
            | 9    | 0.000014 | 0.000037 |      |
        And the ways
            | nodes | oneway |
            | wxyz  | -1     |
            | wv    | yes    |
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1003,1002,1,12
        """
        And the query options
            | scale_factor | <scale> |

        When I request a travel time matrix I should get
            |   | 7          | 8 |
            | 7 | 0          |   |
            | 8 | <duration> | 0 |
        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 7 | 0 |   |
        When I request a travel time matrix I should get
            |   | 7          |
            | 7 | 0          |
            | 8 | <duration> |
        When I request a travel time matrix I should get
            |   | 7          | 8 |
            | 7 | 0          |   |
            | 8 | <duration> | 0 |
            | 9 | 0          |   |

        Examples:
            | scale | duration |
            | 1     | 0.1      |
            | 1.6   | 0.2      |

    # At this speed, each segment has a weight of 1. Thus 7 and 8 have equal
    # rounded weights, but they are 30 m apart.
    Scenario: Points far apart with equal rounded weights keep their order
        Given the node locations
            | node | lon     | lat |
            | l    | 0       | 0   |
            | m    | 0.0009  | 0   |
            | 7    | 0.00027 | 0   |
            | 8    | 0.00054 | 0   |
            | 9    | 0.00027 | 0   |
        And the ways
            | nodes | oneway | maxspeed:forward |
            | lm    | yes    | 10000            |

        When I request a travel distance matrix I should get
            |   | 7 | 8       |
            | 7 | 0 | 30 +- 1 |
            | 8 |   | 0       |
        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 7 | 0 | 0 |
            | 8 |   | 0 |
        When I request a travel distance matrix I should get
            |   | 7 | 8       |
            | 7 | 0 | 30 +- 1 |
        When I request a travel distance matrix I should get
            |   | 7 |
            | 7 | 0 |
            | 8 |   |
        When I request a travel distance matrix I should get
            |   | 7 | 8       |
            | 7 | 0 | 30 +- 1 |
            | 8 |   | 0       |
            | 9 | 0 | 30 +- 1 |

    # 7 and 8 are on opposite sides of the road. Both snap to the same position.
    # Thus the travel between them costs 0 in each direction.
    Scenario: Points on opposite sides of the road share one position
        Given the node locations
            | node | lon    | lat      |
            | p    | -0.001 | 0        |
            | q    | 0.001  | 0        |
            | 7    | 0.0002 | 0.00003  |
            | 8    | 0.0002 | -0.00003 |
        And the ways
            | nodes | oneway |
            | pq    | yes    |

        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 7 | 0 | 0 |
            | 8 | 0 | 0 |
        When I request a travel distance matrix I should get
            |   | 7 | 8 |
            | 7 | 0 | 0 |
            | 8 | 0 | 0 |
        When I request a travel distance matrix I should get
            |   | 7 | 8 |
            | 8 | 0 | 0 |
        When I request a travel distance matrix I should get
            |   | 7 |
            | 7 | 0 |
            | 8 | 0 |

    # b and f have identical coordinates but are distinct road positions. The
    # one-way road goes around the block between them without a junction there.
    Scenario: Returning to the same coordinate does not erase intervening travel
        Given the node locations
            | node | lon   | lat    |
            | f    | 28.91 | 41     |
            | g    | 28.91 | 40.999 |
            | 4    | 28.91 | 41     |
            | 5    | 28.91 | 41     |
            | 6    | 28.91 | 41     |
        And the ways
            | nodes   | highway | oneway |
            | abcedfg | primary | yes    |
        And the query options
            | bearings | 90,10;180,10 |

        When I request a travel distance matrix I should get
            |   | 4 | 5        |
            | 4 | 0 | 390 +- 2 |
            | 5 |   | 0        |
        When I request a travel time matrix I should get
            |   | 4 | 5       |
            | 4 | 0 | 39 +- 1 |
            | 5 |   | 0       |

        Given the query options
            | bearings | 90,10;180,10;180,10 |
        When I request a travel distance matrix I should get
            |   | 4 | 5 |
            | 5 |   | 0 |

        Given the query options
            | bearings | 90,10;90,10;180,10 |
        When I request a travel distance matrix I should get
            |   | 4 |
            | 4 | 0 |
            | 5 |   |

        Given the query options
            | bearings | 90,10;180,10;90,10;180,10;90,10 |
        When I request a travel distance matrix I should get
            |   | 4 | 5        |
            | 4 | 0 | 390 +- 2 |
            | 5 |   | 0        |
            | 6 | 0 | 390 +- 2 |
