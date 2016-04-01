@routing @testbot
Feature: Retrieve geometry

    Background: Use some profile
        Given the profile "testbot"


    @geometry
    Scenario: Route retrieving geometry
       Given the node locations
            | node | lat | lon |
            | a    | 1.0 | 1.5 |
            | b    | 2.0 | 2.5 |
            | c    | 3.0 | 3.5 |
            | d    | 4.0 | 4.5 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        When I route I should get
            | from | to | route    | geometry         |
            | a    | c  | ab,bc,bc | _ibE_~cH_seK_seK |
            | b    | d  | bc,cd,cd | _seK_hgN_seK_seK |

# Mind the \ before the pipes
