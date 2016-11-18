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
            | from | to | route | speed   |
            | a    | b  | ab,ab | 68 km/h |
            | b    | c  | bc,bc | 48 km/h |
            | c    | d  | cd,cd | 40 km/h |
            | d    | e  | de,de | 64 km/h |
            | e    | f  | ef,ef | 80 km/h |
            | f    | g  | fg,fg | 96 km/h |

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
            | from | to | route | speed   |
            | a    | b  | ab,ab | 20 km/h |
            | b    | c  | bc,bc | 72 km/h |
            | c    | d  | cd,cd | 40 km/h |

    Scenario: Car - Forward/backward maxspeed
        Given a grid size of 100 meters

        Then routability should be
            | highway | maxspeed | maxspeed:forward | maxspeed:backward | forw    | backw   |
            | primary |          |                  |                   | 52 km/h | 52 km/h |
            | primary | 60       |                  |                   | 48 km/h | 48 km/h |
            | primary |          | 60               |                   | 48 km/h | 48 km/h +- 5 |
            | primary |          |                  | 60                | 52 km/h | 52 km/h +- 5 |
            | primary | 15       | 60               |                   | 48 km/h | 12 km/h |
            | primary | 15       |                  | 60                | 12 km/h | 48 km/h |
            | primary | 15       | 30               | 60                | 24 km/h | 48 km/h |

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
            | primary |          |       |                  |                   | 52 km/h | 52 km/h |
            | primary |          |   3   |                  |                   | 32 km/h | 32 km/h |
            | primary | 60       |       |                  |                   | 47 km/h | 47 km/h |
            | primary | 60       |   3   |                  |                   | 29 km/h | 29 km/h |
            | primary |          |       | 60               |                   | 47 km/h | 52 km/h |
            | primary |          |   3   | 60               |                   | 29 km/h | 32 km/h |
            | primary |          |       |                  | 60                | 52 km/h | 47 km/h |
            | primary |          |   3   |                  | 60                | 32 km/h | 29 km/h |
            | primary | 15       |       | 60               |                   | 47 km/h | 11 km/h |
            | primary | 15       |   3   | 60               |                   | 29 km/h |  7 km/h |
            | primary | 15       |       |                  | 60                | 12 km/h | 47 km/h |
            | primary | 15       |   3   |                  | 60                |  7 km/h | 29 km/h |
            | primary | 15       |       | 30               | 60                | 23 km/h | 47 km/h |
            | primary | 15       |   3   | 30               | 60                | 14 km/h | 29 km/h |

    Scenario: Car - Single lane streets be ignored or incur a penalty
        Then routability should be

            | highway | maxspeed | lanes | maxspeed:forward | maxspeed:backward | forw    | backw   |
            | primary |          |       |                  |                   | 52 km/h | 52 km/h |
            | primary |          |   1   |                  |                   | 32 km/h | 32 km/h |
            | primary | 60       |       |                  |                   | 47 km/h | 47 km/h |
            | primary | 60       |   1   |                  |                   | 29 km/h | 29 km/h |
            | primary |          |       | 60               |                   | 47 km/h | 52 km/h |
            | primary |          |   1   | 60               |                   | 29 km/h | 32 km/h |
            | primary |          |       |                  | 60                | 52 km/h | 47 km/h |
            | primary |          |   1   |                  | 60                | 32 km/h | 29 km/h |
            | primary | 15       |       | 60               |                   | 47 km/h | 11 km/h |
            | primary | 15       |   1   | 60               |                   | 29 km/h |  7 km/h |
            | primary | 15       |       |                  | 60                | 12 km/h | 47 km/h |
            | primary | 15       |   1   |                  | 60                |  7 km/h | 29 km/h |
            | primary | 15       |       | 30               | 60                | 23 km/h | 47 km/h |
            | primary | 15       |   1   | 30               | 60                | 14 km/h | 29 km/h |

    Scenario: Car - Single lane streets only incure a penalty for two-way streets
        Then routability should be
            | highway | maxspeed | lanes  | oneway | forw    | backw   |
            | primary |   30     |   1    | yes    | 23 km/h |         |
            | primary |   30     |   1    | -1     |         | 23 km/h |
            | primary |   30     |   1    |        | 15 km/h | 15 km/h |
            | primary |   30     |   2    |        | 23 km/h | 23 km/h |

    Scenario: Car - Forwward/backward maxspeed on reverse oneways
        Then routability should be
            | highway | maxspeed | maxspeed:forward | maxspeed:backward | oneway | forw    | backw   |
            | primary |          |                  |                   | -1     |         | 52 km/h |
            | primary | 30       |                  |                   | -1     |         | 23 km/h |
            | primary |          | 30               |                   | -1     |         | 52 km/h |
            | primary |          |                  | 30                | -1     |         | 23 km/h |
            | primary | 20       | 30               |                   | -1     |         | 16 km/h |
            | primary | 20       |                  | 30                | -1     |         | 23 km/h |

