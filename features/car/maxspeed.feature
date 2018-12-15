@routing @maxspeed @car
Feature: Car - Max speed restrictions
OSRM will use 4/5 of the projected free-flow speed.

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Respect maxspeeds when lower than way type speed
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
            | a    | b  | ab,ab | 85 km/h |
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
            | nodes | highway       | maxspeed | # |
            | ab    | residential   |          | default residential speed is 25 |
            | bc    | residential   | 90       |   |
            | cd    | living_street | FR:urban |   |

        When I route I should get
            | from | to | route | speed   |
            | a    | b  | ab,ab | 25 km/h |
                                            # default residential speed is 25, don't mess with this
            | b    | c  | bc,bc | 72 km/h |
                                            # parsed maxspeeds are scaled by profile's speed_reduction value
            | c    | d  | cd,cd | 40 km/h |
                                            # symbolic posted speeds without explicit exceptions are parsed
                                            # from the profile's maxspeed_table_default table

    Scenario: Car - Forward/backward maxspeed are scaled by profile's speed_reduction if explicitly set
        Given a grid size of 100 meters

        Then routability should be
            | highway | maxspeed | maxspeed:forward | maxspeed:backward | forw    | backw   |
            | primary |          |                  |                   | 65 km/h | 65 km/h |
            | primary | 60       |                  |                   | 48 km/h | 48 km/h |
            | primary |          | 60               |                   | 48 km/h | 65 km/h |
            | primary |          |                  | 60                | 65 km/h | 48 km/h |
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

            | highway | maxspeed | width | maxspeed:forward | maxspeed:backward | forw    | backw   | forw_rate | backw_rate |
            | primary |          |       |                  |                   | 64 km/h | 64 km/h | 18        | 18         |
            | primary |          |   3   |                  |                   | 64 km/h | 64 km/h | 9         | 9          |
            | primary | 60       |       |                  |                   | 47 km/h | 47 km/h | 13.3      | 13.3       |
            | primary | 60       |   3   |                  |                   | 47 km/h | 47 km/h | 6.7       | 6.7        |
            | primary |          |       | 60               |                   | 47 km/h | 64 km/h | 13.3      | 18         |
            | primary |          |   3   | 60               |                   | 47 km/h | 64 km/h | 6.7       | 9          |
            | primary |          |       |                  | 60                | 64 km/h | 47 km/h | 18        | 13.3       |
            | primary |          |   3   |                  | 60                | 64 km/h | 47 km/h | 9         | 6.7        |
            | primary | 15       |       | 60               |                   | 47 km/h | 11 km/h | 13.3      | 3.3        |
            | primary | 15       |   3   | 60               |                   | 48 km/h | 12 km/h | 6.7       | 1.7        |
            | primary | 15       |       |                  | 60                | 12 km/h | 47 km/h | 3.3       | 13.3       |
            | primary | 15       |   3   |                  | 60                | 12 km/h | 47 km/h | 1.7       | 6.7        |
            | primary | 15       |       | 30               | 60                | 23 km/h | 47 km/h | 6.7       | 13.3       |
            | primary | 15       |   3   | 30               | 60                | 23 km/h | 47 km/h | 3.3       | 6.7        |

    Scenario: Car - Single lane streets be ignored or incur a penalty
        Then routability should be

            | highway | maxspeed | lanes | maxspeed:forward | maxspeed:backward | forw    | backw   | forw_rate | backw_rate |
            | primary |          |       |                  |                   | 64 km/h | 64 km/h | 18        | 18         |
            | primary |          |   1   |                  |                   | 64 km/h | 64 km/h | 9         | 9          |
            | primary | 60       |       |                  |                   | 47 km/h | 47 km/h | 13.3      | 13.3       |
            | primary | 60       |   1   |                  |                   | 47 km/h | 47 km/h | 6.7       | 6.7        |
            | primary |          |       | 60               |                   | 47 km/h | 64 km/h | 13.3      | 18         |
            | primary |          |   1   | 60               |                   | 47 km/h | 64 km/h | 6.7       | 9          |
            | primary |          |       |                  | 60                | 64 km/h | 47 km/h | 18        | 13.3       |
            | primary |          |   1   |                  | 60                | 64 km/h | 47 km/h | 9         | 6.7        |
            | primary | 15       |       | 60               |                   | 47 km/h | 11 km/h | 13.3      | 3.3        |
            | primary | 15       |   1   | 60               |                   | 48 km/h | 12 km/h | 6.7       | 1.7        |
            | primary | 15       |       |                  | 60                | 12 km/h | 47 km/h | 3.3       | 13.3       |
            | primary | 15       |   1   |                  | 60                | 12 km/h | 47 km/h | 1.7       | 6.7        |
            | primary | 15       |       | 30               | 60                | 23 km/h | 47 km/h | 6.7       | 13.3       |
            | primary | 15       |   1   | 30               | 60                | 23 km/h | 47 km/h | 3.3       | 6.7        |

    Scenario: Car - Single lane streets only incur a penalty for two-way streets
        Then routability should be
            | highway | maxspeed | lanes  | oneway | forw    | backw   | forw_rate | backw_rate |
            | primary |   30     |   1    | yes    | 23 km/h |         | 6.7       |            |
            | primary |   30     |   1    | -1     |         | 23 km/h |           | 6.7        |
            | primary |   30     |   1    |        | 23 km/h | 23 km/h | 3.3       | 3.3        |
            | primary |   30     |   2    |        | 23 km/h | 23 km/h | 6.7       | 6.7        |

    Scenario: Car - Forward/backward maxspeed on reverse oneways
        Then routability should be
            | highway | maxspeed | maxspeed:forward | maxspeed:backward | oneway | forw    | backw   | forw_rate | backw_rate |
            | primary |          |                  |                   | -1     |         | 64 km/h |           | 18         |
            | primary | 30       |                  |                   | -1     |         | 23 km/h |           | 6.7        |
            | primary |          | 30               |                   | -1     |         | 64 km/h |           | 18         |
            | primary |          |                  | 30                | -1     |         | 23 km/h |           | 6.7        |
            | primary | 20       | 30               |                   | -1     |         | 15 km/h |           | 4.4        |
            | primary | 20       |                  | 30                | -1     |         | 23 km/h |           | 6.7        |


    Scenario: Car - Respect source:maxspeed
        Given the node map
            """
            a b c d e f g
            """

        And the ways
            | nodes | highway | source:maxspeed    | maxspeed |
            | ab    | trunk   |                    |          |
            | bc    | trunk   |                    | 60       |
            | cd    | trunk   | FR:urban           |          |
            | de    | trunk   | CH:rural           |          |
            | ef    | trunk   | CH:trunk           |          |
            | fg    | trunk   | CH:motorway        |          |

        When I route I should get
            | from | to | route | speed   |
            | a    | b  | ab,ab | 85 km/h |
            | b    | c  | bc,bc | 48 km/h |
            | c    | d  | cd,cd | 40 km/h |
            | d    | e  | de,de | 64 km/h |
            | e    | f  | ef,ef | 80 km/h |
            | f    | g  | fg,fg | 96 km/h |