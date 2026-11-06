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
