@nearest
Feature: Nearest API - node IDs in response

    Background:
        Given the profile "testbot"

    Scenario: Nearest - one-way road snap at start returns valid non-zero node IDs
        Given the node map
            """
            0 a x b
            """

        And the ways
            | nodes | oneway |
            | axb   | yes    |

        When I request nearest I should get
            | in | out | nodes |
            | 0  | a   | a,x   |

