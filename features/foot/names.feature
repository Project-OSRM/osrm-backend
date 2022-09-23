@routing @foot @names
Feature: Foot - Street names in instructions

    Background:
        Given the profile "foot"
        Given a grid size of 200 meters

    Scenario: Foot - A named street
        Given the node map
            """
            a b
              c
            """

        And the ways
            | nodes | name     | ref |
            | ab    | My Way   |     |
            | bc    |          | A7  |

        When I route I should get
            | from | to | route    | ref    |
            | a    | c  | My Way,, | ,A7,A7 |


    Scenario: Foot - Combines named roads with suffix changes
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | name           |
            | ab    | High Street W  |
            | bc    | High Street E  |
            | cd    | Market Street  |

        When I route I should get
            | from | to | route                                     |
            | a    | d  | High Street W,Market Street,Market Street |
