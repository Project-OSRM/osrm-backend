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


    # Direct test for the diagram in PR #7442: a single bidirectional road between
    # two nodes. After contraction the edge resides at the lower-numbered node (0).
    # Both waypoints snap to node 1, source downstream from target. Without self-loops
    # the forward search cannot leave node 1, so no route is found.
    Scenario: Waypoints snap to same node, source downstream
        Given the node map
            """
             1   2
            a-----b
            """

        And the ways
            | nodes | highway     |
            | ab    | residential |

        When I route I should get
            | waypoints | approaches | route       |
            | 1,2       | curb curb  | ab,ab,ab,ab |
            | 2,1       | curb curb  | ab,ab       |
