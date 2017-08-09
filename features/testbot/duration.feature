@routing @testbot @routes @duration
Feature: Durations

    Background:
        Given the profile "testbot"

    Scenario: Duration of ways
        Given the node map
            """
            a b       f
                  e
              c     d
            """

        And the ways
            | nodes | highway | duration |
            | ab    | primary | 0:01     |
            | bc    | primary | 0:10     |
            | cd    | primary | 1:00     |
            | de    | primary | 10:00    |
            | ef    | primary | 01:02:03 |

        When I route I should get
            | from | to | route | time       |
            | a    | b  | ab,ab | 60s +-1    |
            | b    | c  | bc,bc | 600s +-1   |
            | c    | d  | cd,cd | 3600s +-1  |
            | d    | e  | de,de | 36000s +-1 |
            | e    | f  | ef,ef | 3723s +-1  |

    @todo
    Scenario: Partial duration of ways
        Given the node map
            """
            a b   c
            """

        And the ways
            | nodes | highway | duration |
            | abc   | primary | 0:01     |

        When I route I should get
            | from | to | route   | time    |
            | a    | c  | abc,abc | 60s +-1 |
            | a    | b  | ab,ab   | 20s +-1 |
            | b    | c  | bc,bc   | 40s +-1 |
