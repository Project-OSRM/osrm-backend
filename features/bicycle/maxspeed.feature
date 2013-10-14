@routing @maxspeed @bicycle
Feature: Bike - Max speed restrictions

    Background: Use specific speeds
        Given the profile "bicycle"

    Scenario: Bicycle - Respect maxspeeds when lower that way type speed
        Then routability should be
            | highway     | maxspeed | bothw    |
            | residential |          | 49s ~10% |
            | residential | 10       | 72s ~10% |

    Scenario: Bicycle - Ignore maxspeed when higher than way speed
        Then routability should be
            | highway     | maxspeed | bothw    |
            | residential |          | 49s ~10% |
            | residential | 80       | 49s ~10% |

    @todo
    Scenario: Bicycle - Maxspeed formats
        Then routability should be
            | highway     | maxspeed  | bothw     |
            | residential |           | 49s ~10%  |
            | residential | 5         | 144s ~10% |
            | residential | 5mph      | 90s ~10%  |
            | residential | 5 mph     | 90s ~10%  |
            | residential | 5MPH      | 90s ~10%  |
            | residential | 5 MPH     | 90s ~10%  |
            | trunk       | 5unknown  | 49s ~10%  |
            | trunk       | 5 unknown | 49s ~10%  |

    @todo
    Scenario: Bicycle - Maxspeed special tags
        Then routability should be
            | highway     | maxspeed | bothw    |
            | residential |          | 49s ~10% |
            | residential | none     | 49s ~10% |
            | residential | signals  | 49s ~10% |

    Scenario: Bike - Do not use maxspeed when higher that way type speed
        Given the node map
            | a | b | c |

        And the ways
            | nodes | highway     | maxspeed |
            | ab    | residential |          |
            | bc    | residential | 80       |

        When I route I should get
            | from | to | route | time    |
            | a    | b  | ab    | 24s ~5% |
            | b    | c  | bc    | 24s ~5% |

    Scenario: Bike - Forward/backward maxspeed
        Given the shortcuts
            | key   | value     |
            | bike  | 49s ~10%  |
            | run   | 73s ~10%  |
            | walk  | 145s ~10% |
            | snail | 720s ~10% |

        Then routability should be
            | maxspeed | maxspeed:forward | maxspeed:backward | forw  | backw |
            |          |                  |                   | bike  | bike  |
            | 10       |                  |                   | run   | run   |
            |          | 10               |                   | run   | bike  |
            |          |                  | 10                | bike  | run   |
            | 1        | 10               |                   | run   | snail |
            | 1        |                  | 10                | snail | run   |
            | 1        | 5                | 10                | walk  | run   |

    Scenario: Bike - Maxspeed should not allow routing on unroutable ways
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
