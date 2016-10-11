@routing @maxspeed @car
Feature: Car - Max speed restrictions
OSRM will use 4/5 of the projected free-flow speed.

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Respect maxspeeds when lower that way type speed
        Given the node map
            """
            a b c d e f g
            """

        And the ways
            | nodes | highway | maxspeed    |
            | ab    | trunk   |             |
            | bc    | trunk   | 60          |
            | cd    | trunk   | FR:urban    |
            | de    | trunk   | CH:rural    |
            | ef    | trunk   | CH:trunk    |
            | fg    | trunk   | CH:motorway |

        When I route I should get
            | from | to | route | speed         |
            | a    | b  | ab,ab |  79 km/h      |
            | b    | c  | bc,bc |  59 km/h +- 1 |
            | c    | d  | cd,cd |  51 km/h      |
            | d    | e  | de,de |  75 km/h      |
            | e    | f  | ef,ef |  91 km/h      |
            | f    | g  | fg,fg | 107 km/h      |

    Scenario: Car - Do not ignore maxspeed when higher than way speed
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway       | maxspeed |
            | ab    | residential   |          |
            | bc    | residential   | 90       |
            | cd    | living_street | FR:urban |

        When I route I should get
            | from | to | route | speed        |
            | a    | b  | ab,ab | 31 km/h      |
            | b    | c  | bc,bc | 83 km/h +- 1 |
            | c    | d  | cd,cd | 51 km/h      |

    Scenario: Car - Forward/backward maxspeed
        Given a grid size of 100 meters

        Then routability should be
            | highway | maxspeed | maxspeed:forward | maxspeed:backward | forw         | backw        |
            | primary |          |                  |                   | 63 km/h      | 63 km/h      |
            | primary | 60       |                  |                   | 60 km/h +- 1 | 60 km/h +- 1 |
            | primary |          | 60               |                   | 60 km/h +- 1 | 63 km/h      |
            | primary |          |                  | 60                | 63 km/h      | 60 km/h +- 1 |
            | primary | 15       | 60               |                   | 60 km/h +- 1 | 23 km/h      |
            | primary | 15       |                  | 60                | 23 km/h +- 1 | 60 km/h +- 1 |
            | primary | 15       | 30               | 60                | 34 km/h +- 1 | 60 km/h +- 1 |

    Scenario: Car - Maxspeed should not allow routing on unroutable ways
        Then routability should be
            | highway   | railway | access | maxspeed | maxspeed:forward | maxspeed:backward | bothw |
            | primary   |         |        |          |                  |                   | x     |
            | secondary |         | no     |          |                  |                   |       |
            | secondary |         | no     | 100      |                  |                   |       |
            | secondary |         | no     |          | 100              |                   |       |
            | secondary |         | no     |          |                  | 100               |       |
            | (nil)     | train   |        |          |                  |                   |       |
            | (nil)     | train   |        | 100      |                  |                   |       |
            | (nil)     | train   |        |          | 100              |                   |       |
            | (nil)     | train   |        |          |                  | 100               |       |
            | runway    |         |        |          |                  |                   |       |
            | runway    |         |        | 100      |                  |                   |       |
            | runway    |         |        |          | 100              |                   |       |
            | runway    |         |        |          |                  | 100               |       |

    Scenario: Car - Too narrow streets should be ignored or incur a penalty
        Then routability should be

            | highway | maxspeed | width | maxspeed:forward | maxspeed:backward | forw    | backw   |
            | primary |          |       |                  |                   | 63 km/h | 63 km/h |
            | primary |          |   3   |                  |                   | 32 km/h | 32 km/h |
            | primary | 60       |       |                  |                   | 59 km/h | 59 km/h |
            | primary | 60       |   3   |                  |                   | 29 km/h | 29 km/h |
            | primary |          |       | 60               |                   | 59 km/h | 63 km/h |
            | primary |          |   3   | 60               |                   | 29 km/h | 32 km/h |
            | primary |          |       |                  | 60                | 63 km/h | 59 km/h |
            | primary |          |   3   |                  | 60                | 32 km/h | 29 km/h |
            | primary | 15       |       | 60               |                   | 59 km/h | 23 km/h |
            | primary | 15       |   3   | 60               |                   | 29 km/h |  7 km/h |
            | primary | 15       |       |                  | 60                | 22 km/h | 59 km/h |
            | primary | 15       |   3   |                  | 60                |  7 km/h | 29 km/h |
            | primary | 15       |       | 30               | 60                | 35 km/h | 59 km/h |
            | primary | 15       |   3   | 30               | 60                | 14 km/h | 29 km/h |

    Scenario: Car - Single lane streets be ignored or incur a penalty
        Then routability should be

            | highway | maxspeed | lanes | maxspeed:forward | maxspeed:backward | forw    | backw   |
            | primary |          |       |                  |                   | 63 km/h | 63 km/h |
            | primary |          |   1   |                  |                   | 32 km/h | 32 km/h |
            | primary | 60       |       |                  |                   | 59 km/h | 59 km/h |
            | primary | 60       |   1   |                  |                   | 29 km/h | 29 km/h |
            | primary |          |       | 60               |                   | 59 km/h | 63 km/h |
            | primary |          |   1   | 60               |                   | 29 km/h | 32 km/h |
            | primary |          |       |                  | 60                | 63 km/h | 59 km/h |
            | primary |          |   1   |                  | 60                | 32 km/h | 29 km/h |
            | primary | 15       |       | 60               |                   | 59 km/h | 23 km/h |
            | primary | 15       |   1   | 60               |                   | 29 km/h |  7 km/h |
            | primary | 15       |       |                  | 60                | 22 km/h | 59 km/h |
            | primary | 15       |   1   |                  | 60                |  7 km/h | 29 km/h |
            | primary | 15       |       | 30               | 60                | 35 km/h | 59 km/h |
            | primary | 15       |   1   | 30               | 60                | 14 km/h | 29 km/h |

    Scenario: Car - Single lane streets only incure a penalty for two-way streets
        Then routability should be
            | highway | maxspeed | lanes  | oneway | forw    | backw   |
            | primary |   30     |   1    | yes    | 35 km/h |         |
            | primary |   30     |   1    | -1     |         | 35 km/h |
            | primary |   30     |   1    |        | 15 km/h | 15 km/h |
            | primary |   30     |   2    |        | 35 km/h | 35 km/h |
