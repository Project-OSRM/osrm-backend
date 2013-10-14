@routing @maxspeed @testbot
Feature: Car - Max speed restrictions

    Background: Use specific speeds
        Given the profile "testbot"

    Scenario: Testbot - Respect maxspeeds when lower that way type speed
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | maxspeed |
            | ab    |          |
            | bc    | 24       |
            | cd    | 18       |

        When I route I should get
            | from | to | route | time    |
            | a    | b  | ab    | 10s +-1 |
            | b    | a  | ab    | 10s +-1 |
            | b    | c  | bc    | 15s +-1 |
            | c    | b  | bc    | 15s +-1 |
            | c    | d  | cd    | 20s +-1 |
            | d    | c  | cd    | 20s +-1 |

    Scenario: Testbot - Ignore maxspeed when higher than way speed
        Given the node map
            | a | b | c |

        And the ways
            | nodes | maxspeed |
            | ab    |          |
            | bc    | 200      |

        When I route I should get
            | from | to | route | time    |
            | a    | b  | ab    | 10s +-1 |
            | b    | a  | ab    | 10s +-1 |
            | b    | c  | bc    | 10s +-1 |
            | c    | b  | bc    | 10s +-1 |

    @opposite
    Scenario: Testbot - Forward/backward maxspeed
        Then routability should be
            | maxspeed | maxspeed:forward | maxspeed:backward | forw    | backw   |
            |          |                  |                   | 20s +-1 | 20s +-1 |
            | 18       |                  |                   | 40s +-1 | 40s +-1 |
            |          | 18               |                   | 40s +-1 | 20s +-1 |
            |          |                  | 18                | 20s +-1 | 40s +-1 |
            | 9        | 18               |                   | 40s +-1 | 80s +-1 |
            | 9        |                  | 18                | 80s +-1 | 40s +-1 |
            | 9        | 24               | 18                | 30s +-1 | 40s +-1 |
