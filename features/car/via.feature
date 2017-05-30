@routing @via
Feature: Via points

    Background:
        Given the profile "car"

    # See issue #1896
    Scenario: Via point at a dead end with barrier
        Given the profile "car"
        Given the node map
            """
            a b c
              1
              d


            f e
            """

        And the nodes
            | node | barrier |
            | d    | bollard |

        And the ways
            | nodes |
            | abc   |
            | bd    |
            | afed  |

        When I route I should get
            | waypoints | route                   |
            | a,1,c     | abc,bd,bd,bd,bd,abc,abc |
            | c,1,a     | abc,bd,bd,bd,bd,abc,abc |
