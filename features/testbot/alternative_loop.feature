@routing @testbot @alternative
Feature: Alternative route

    Background:
        Given the profile "testbot"
        Given a grid size of 200 meters

    Scenario: Alternative Loop Paths
        Given the node map
            """
            a 2 1 b
            7     4
            8     3
            c 5 6 d
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bd    | yes    |
            | dc    | yes    |
            | ca    | yes    |

        And the query options
            | alternatives | true |

        When I route I should get
            | from | to | route             | alternative |
            | 1    | 2  | ab,bd,dc,ca,ab,ab |             |
            | 3    | 4  | bd,dc,ca,ab,bd,bd |             |
            | 5    | 6  | dc,ca,ab,bd,dc,dc |             |
            | 7    | 8  | ca,ab,bd,dc,ca,ca |             |
