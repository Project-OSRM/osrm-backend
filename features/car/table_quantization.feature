@routing @car @matrix
Feature: Table routes between nearby points with equal rounded weights

    Background:
        Given the profile "car"
        And the node locations
            | node | lon       | lat   |
            | a    | 28.90     | 41    |
            | b    | 28.91     | 41    |
            | c    | 28.92     | 41    |
            | d    | 28.93     | 41    |
            | 1    | 28.914    | 41    |
            | 2    | 28.914002 | 41    |
            | 3    | 28.914    | 41    |
            | 6    | 28.914001 | 41    |

    # The side-road penalty makes weight and duration round at different positions.
    # The two points are reachable in either direction, about 0.17 metres apart.
    Scenario Outline: Nearby reachable points retain positive durations when scaled
        Given the ways
            | nodes | highway     | side_road |
            | ab    | residential |           |
            | bc    | residential | yes       |
            | cd    | residential |           |
        And the query options
            | scale_factor | <scale> |

        When I request a travel time matrix I should get
            |   | 1          | 2          |
            | 1 | 0          | <duration> |
            | 2 | <duration> | 0          |
        When I request a travel time matrix I should get
            |   | 1 | 2          |
            | 1 | 0 | <duration> |
        When I request a travel time matrix I should get
            |   | 1          |
            | 1 | 0          |
            | 2 | <duration> |
        When I request a travel time matrix I should get
            |   | 1          | 2          |
            | 1 | 0          | <duration> |
            | 2 | <duration> | 0          |
            | 3 | 0          | <duration> |
        When I request a travel distance matrix I should get
            |   | 1   | 2   |
            | 1 | 0   | 0.2 |
            | 2 | 0.2 | 0   |

        Examples:
            | scale | duration |
            | 1     | 0.1      |
            | 1.6   | 0.2      |

    Scenario: Nearby points with zero rounded duration still have positive distance
        Given the ways
            | nodes | highway     |
            | ab    | residential |
            | bc    | residential |
            | cd    | residential |

        When I request a travel time matrix I should get
            |   | 1 | 6 |
            | 1 | 0 | 0 |
            | 6 | 0 | 0 |
        When I request a travel distance matrix I should get
            |   | 1   | 6   |
            | 1 | 0   | 0.1 |
            | 6 | 0.1 | 0   |
        When I request a travel distance matrix I should get
            |   | 1 | 6   |
            | 1 | 0 | 0.1 |
        When I request a travel distance matrix I should get
            |   | 1   |
            | 1 | 0   |
            | 6 | 0.1 |

    Scenario: Equal rounded weights do not allow travel against a one-way road
        Given the ways
            | nodes | highway     | oneway | side_road |
            | ab    | residential | yes    |           |
            | bc    | residential | yes    | yes       |
            | cd    | residential | yes    |           |
        And the query options
            | scale_factor | 1.6 |

        When I request a travel time matrix I should get
            |   | 1 | 2   |
            | 1 | 0 | 0.2 |
            | 2 |   | 0   |
        When I request a travel time matrix I should get
            |   | 1 | 2 |
            | 2 |   | 0 |
        When I request a travel time matrix I should get
            |   | 1 |
            | 1 | 0 |
            | 2 |   |
        When I request a travel distance matrix I should get
            |   | 1 | 2   |
            | 1 | 0 | 0.2 |
            | 2 |   | 0   |

    # Drawn against the travel direction, so the searches run on reverse segments.
    Scenario: Equal rounded weights do not allow travel against a reversed one-way road
        Given the ways
            | nodes | highway     | oneway | side_road |
            | ab    | residential | -1     |           |
            | bc    | residential | -1     | yes       |
            | cd    | residential | -1     |           |
        And the query options
            | scale_factor | 1.6 |

        When I request a travel time matrix I should get
            |   | 1   | 2 |
            | 1 | 0   |   |
            | 2 | 0.2 | 0 |
        When I request a travel time matrix I should get
            |   | 1 | 2 |
            | 1 | 0 |   |
        When I request a travel time matrix I should get
            |   | 2 |
            | 1 |   |
            | 2 | 0 |
        When I request a travel distance matrix I should get
            |   | 1   | 2 |
            | 1 | 0   |   |
            | 2 | 0.2 | 0 |

    Scenario: Nearby points on a one-way loop take the reachable detour
        Given the node locations
            | node | lon       | lat   |
            | e    | 29.91     | 41    |
            | f    | 29.92     | 41    |
            | g    | 29.92     | 41.01 |
            | h    | 29.91     | 41.01 |
            | 4    | 29.914    | 41    |
            | 5    | 29.914002 | 41    |
        And the ways
            | nodes | highway     | oneway | side_road |
            | ef    | residential | yes    | yes       |
            | fg    | residential | yes    |           |
            | gh    | residential | yes    |           |
            | he    | residential | yes    |           |

        When I request a travel distance matrix I should get
            |   | 4          | 5   |
            | 4 | 0          | 0.2 |
            | 5 | 3900 +- 50 | 0   |
        When I request a travel distance matrix I should get
            |   | 4          | 5 |
            | 5 | 3900 +- 50 | 0 |
        When I request a travel distance matrix I should get
            |   | 4          |
            | 4 | 0          |
            | 5 | 3900 +- 50 |

    # The weight is the distance, so the signal at q adds duration but no weight.
    # 7 and 8 are both at q: 7 is before the signal and 8 is after it. The route
    # from 8 to 7 must go back through the signal, so it is not possible.
    Scenario: A traffic signal between two points at one node keeps their order
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'distance'
        """
        And the node locations
            | node | lon    | lat   |
            | p    | -0.001 | 0     |
            | q    | 0      | 0     |
            | r    | 0.001  | 0.001 |
            | 7    | 0      | 0     |
            | 8    | 0      | 0     |
            | 9    | 0      | 0     |
        And the nodes
            | node | highway         |
            | q    | traffic_signals |
        And the ways
            | nodes | oneway |
            | pqr   | yes    |
        And the query options
            | bearings | 90,10;45,10 |

        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 7 | 0 | 2 |
            | 8 |   | 0 |
        When I request a travel distance matrix I should get
            |   | 7 | 8 |
            | 7 | 0 | 0 |
            | 8 |   | 0 |

        # Non-square requests list destination coordinates before source coordinates.
        Given the query options
            | bearings | 90,10;45,10;45,10 |
        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 8 |   | 0 |

        Given the query options
            | bearings | 90,10;90,10;45,10 |
        When I request a travel time matrix I should get
            |   | 7 |
            | 7 | 0 |
            | 8 |   |

        Given the query options
            | bearings | 90,10;45,10;90,10;45,10;90,10 |
        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 7 | 0 | 2 |
            | 8 |   | 0 |
            | 9 | 0 | 2 |

    # The same road with oneway=-1, so traffic moves from r to p. Now 8 is before
    # the signal and 7 is after it. The route from 7 to 8 is not possible.
    Scenario: A traffic signal between two points at one node keeps their order on a reversed one-way road
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'distance'
        """
        And the node locations
            | node | lon    | lat   |
            | p    | -0.001 | 0     |
            | q    | 0      | 0     |
            | r    | 0.001  | 0.001 |
            | 7    | 0      | 0     |
            | 8    | 0      | 0     |
        And the nodes
            | node | highway         |
            | q    | traffic_signals |
        And the ways
            | nodes | oneway |
            | pqr   | -1     |
        And the query options
            | bearings | 270,10;225,10 |

        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 7 | 0 |   |
            | 8 | 2 | 0 |

        Given the query options
            | bearings | 270,10;225,10;270,10 |
        When I request a travel time matrix I should get
            |   | 7 | 8 |
            | 7 | 0 |   |

        Given the query options
            | bearings | 225,10;270,10;225,10 |
        When I request a travel time matrix I should get
            |   | 8 |
            | 7 |   |
            | 8 | 0 |
