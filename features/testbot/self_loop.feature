@routing @testbot @self-loop
Feature: Self-loops
    Background:
        Given the profile "testbot"

    Scenario: Waypoints on same edge with approaches
        Given the node map
            """
              4   3
            a-------b
              1   2
            """

        And the ways
            | nodes | highway     |
            | ab    | residential |

        When I route I should get
            | waypoints | approaches | route       |
            | 1,2       | curb curb  | ab,ab       |
            | 2,3       | curb curb  | ab,ab,ab    |
            | 3,4       | curb curb  | ab,ab       |
            | 4,1       | curb curb  | ab,ab,ab    |
            | 4,3       | curb curb  | ab,ab,ab,ab |
            | 3,2       | curb curb  | ab,ab,ab    |
            | 2,1       | curb curb  | ab,ab,ab,ab |
            | 1,4       | curb curb  | ab,ab,ab    |


    Scenario: Waypoints on same edge with bearings
        Given the node map
            """
              4   3
            a-------b
              1   2
            """

        And the ways
            | nodes | highway     |
            | ab    | residential |

        When I route I should get
            | waypoints | bearings | route       |
            | 2,1       |  90  90  | ab,ab,ab,ab |
            | 4,3       | 270 270  | ab,ab,ab,ab |
