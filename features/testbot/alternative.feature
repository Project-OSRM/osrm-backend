@routing @testbot @alternative
Feature: Alternative route

    Background:
        Given the profile "testbot"
        And a grid size of 200 meters
        # Force data preparation to single-threaded to ensure consistent
        # results for alternative generation during tests (alternative
        # finding is highly sensitive to graph shape, which is in turn
        # affected by parallelism during generation)
        And the contract extra arguments "--threads 1"
        And the extract extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"
        And the partition extra arguments "--threads 1"

        And the node map
            """
              b c d
            a   k     z
              g h i j
            """

        # enforce multiple cells for filterUnpackedPathsBySharing check
        And the partition extra arguments "--small-component-size 1 --max-cell-sizes 2,4,8,16"

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | dz    |
            | ag    |
            | gh    |
            | ck    |
            | kh    |
            | hi    |
            | ij    |
            | jz    |

    Scenario: Enabled alternative
        Given the query options
            | alternatives | true |

        When I route I should get
            | from | to | route          | alternative       |
            | a    | z  | ab,bc,cd,dz,dz | ag,gh,hi,ij,jz,jz |

    Scenario: Disabled alternative
        Given the query options
            | alternatives | false |

        When I route I should get
            | from | to | route          | alternative |
            | a    | z  | ab,bc,cd,dz,dz |             |
