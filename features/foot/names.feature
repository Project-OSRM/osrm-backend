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
