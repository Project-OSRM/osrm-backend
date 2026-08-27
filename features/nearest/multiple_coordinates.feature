@nearest
Feature: Nearest API - multiple coordinates in a single request

    Background:
        Given the profile "testbot"

    Scenario: Nearest - multiple coordinates, all matched
        Given the node map
            """
            0 a x b
            """

        And the ways
            | nodes | oneway |
            | axb   | yes    |

        When I request nearest for multiple coordinates I should get
            | in | out |
            | 0  | a   |
            | 0  | a   |

    Scenario: Nearest - multiple coordinates, one unmatched
        Given the node map
            """
            0     1
            a     x     b
            """

        And the ways
            | nodes | oneway |
            | axb   | yes    |

        When I request nearest for multiple coordinates I should get
            | in | out       | radius |
            | 0  | a         |        |
            | 1  | NoSegment | 1      |
