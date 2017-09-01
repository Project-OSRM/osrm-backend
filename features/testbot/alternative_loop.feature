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

    # This test case does not work in a platform independent way
    # since it depends on a specific CH structure that is only
    # present on linux it seems.
    @4111 @todo
    Scenario: Alternative Loop Paths with single node path
        Given the node map
            """
            a1b2c3d


            e     f
            """

        And the ways
            | nodes | maxspeed |
            | ab    |      30  |
            | bc    |       3  |
            | cd    |      30  |
            | ae    |      30  |
            | ef    |      30  |
            | fd    |      30  |

        And the query options
            | alternatives | true |

        When I route I should get
            | from | to | route    | alternative       |
            | b    | c  | bc,bc    | ab,ae,ef,fd,cd,cd |
            #| c    | b  | bc,bc    | cd,fd,ef,ae,ab,ab | # alternative path depends on phantom snapping order
            | 1    | c  | ab,bc,bc | ab,ae,ef,fd,cd,cd |
            #| c    | 1  | bc,ab    | cd,fd,ef,ae,ab    | # alternative path depends on phantom snapping order
            | 2 | c | bc,bc          |                   |
            | c | 2 | bc,bc          |                   |
            | 1 | 3 | ab,ae,ef,fd,cd | ab,bc,cd          |
            #| 3 | 1 | cd,fd,ef,ae,ab | cd,bc,ab          | # alternative path depends on phantom snapping order
            | b | 3 | bc,cd          | ab,ae,ef,fd,cd    |
            #| 3 | b | cd,bc,bc       | cd,fd,ef,ae,ab,ab | # alternative path depends on phantom snapping order
