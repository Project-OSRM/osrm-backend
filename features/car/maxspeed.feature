@routing @maxspeed @car
Feature: Car - Max speed restrictions

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Respect maxspeeds when lower that way type speed
        Given the node map
            | a | b | c |

        And the ways
            | nodes | highway | maxspeed |
            | ab    | trunk   |          |
            | bc    | trunk   | 10       |

        When I route I should get
            | from | to | route | time      |
            | a    | b  | ab    | 42s ~10%  |
            | b    | c  | bc    | 360s ~10% |

    Scenario: Car - Do not ignore maxspeed when higher than way speed
        Given the node map
            | a | b | c |

        And the ways
            | nodes | highway     | maxspeed |
            | ab    | residential |          |
            | bc    | residential | 85       |

        When I route I should get
            | from | to | route | time      |
            | a    | b  | ab    | 144s ~10% |
            | b    | c  | bc    | 42s ~10%  |

    Scenario: Car - Forward/backward maxspeed
        Given the shortcuts
            | key   | value     |
            | car   | 12s ~10%  |
            | run   | 73s ~10%  |
            | walk  | 146s ~10% |
            | snail | 720s ~10% |

        And a grid size of 100 meters

        Then routability should be
            | maxspeed | maxspeed:forward | maxspeed:backward | forw  | backw |
            |          |                  |                   | car   | car   |
            | 10       |                  |                   | run   | run   |
            |          | 10               |                   | run   | car   |
            |          |                  | 10                | car   | run   |
            | 1        | 10               |                   | run   | snail |
            | 1        |                  | 10                | snail | run   |
            | 1        | 5                | 10                | walk  | run   |

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
