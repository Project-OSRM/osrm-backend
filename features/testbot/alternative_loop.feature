@routing @testbot @alternative
Feature: Alternative route

    Background:
        Given the profile "testbot"
        Given a grid size of 200 meters
        # Force data preparation to single-threaded to ensure consistent
        # results for alternative generation during tests (alternative
        # finding is highly sensitive to graph shape, which is in turn
        # affected by parallelism during generation)
        And the contract extra arguments "--threads 1"
        And the extract extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"
        And the partition extra arguments "--threads 1"

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


    @mld-only
    Scenario: Alternative loop paths on a single node with an asymmetric circle
        # The test checks only MLD implementation, alternatives results are unpredictable for CH on windows (#4691, #4693)
        Given a grid size of 10 meters
        Given the node map
            """
              a b  c
            l       d
            k       e
            j       f
              i h g
            """

        And the nodes
            | node | barrier |
            | i    | bollard |
            | g    | bollard |

        And the ways
            | nodes         | oneway |
            | abcdefghijkla | no     |

        And the query options
            | alternatives | true |

        When I route I should get
            | from | to | route                       | alternative                 | weight |
            | e    | k  | abcdefghijkla,abcdefghijkla | abcdefghijkla,abcdefghijkla |    6.8 |
