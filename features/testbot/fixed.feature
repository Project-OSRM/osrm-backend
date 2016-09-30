@routing @testbot @fixed
Feature: Fixed bugs, kept to check for regressions

    Background:
        Given the profile "testbot"

    @726
    Scenario: Weird looping, manual input
        Given the node locations
            | node | lat       | lon       |
            | a    | 55.660778 | 12.573909 |
            | b    | 55.660672 | 12.573693 |
            | c    | 55.660128 | 12.572546 |
            | d    | 55.660015 | 12.572476 |
            | e    | 55.660119 | 12.572325 |
            | x    | 55.660818 | 12.574051 |
            | y    | 55.660073 | 12.574067 |

        And the ways
            | nodes |
            | abc   |
            | cdec  |

        When I route I should get
            | from | to | route   |
            | x    | y  | abc,abc |

    Scenario: Step trimming with very short segments
        Given a grid size of 0.1 meters
        Given the node map
            """
            a 1 b c d 2 e
            """

        Given the ways
            | nodes | oneway |
            | ab    | yes    |
            | bcd   | yes    |
            | de    | yes    |

        When I route I should get
            | from | to | route     |
            | 1    | 2  | bcd,bcd   |
