@routing @testbot @routes @snapping
Feature: Name hints when snapping

    Background:
        Given the profile "testbot"

    Scenario: Snap to nearby named roads, instead of the closest

        Given the node map
            """
            a-----------b


              1       2
            c-----------d
            """

        And the ways
            | nodes | name    |
            | ab    | test st |
            | cd    | side st |

        When I route I should get
            | from | to | name_hints | route            |
            | 1    | 2  | test,test  | test st,test st  |
            | 1    | 2  | side,side  | side st,side st  |
