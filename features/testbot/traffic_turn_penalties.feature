@routing @speed @traffic
Feature: Traffic - turn penalties applied to turn onto which a phantom node snaps

    Background: Simple map with phantom nodes
        Given the node map
            """
              1   2   3
            a   b   c   d

                e   f   g
            """

        And the nodes
            | node | id |
            | a    | 1  |
            | b    | 2  |
            | c    | 3  |
            | d    | 4  |
            | e    | 5  |
            | f    | 6  |
            | g    | 7  |

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | cd    | primary |

            | be    | primary |
            | cf    | primary |
            | dg    | primary |
        And the profile "testbot"
        # Since testbot doesn't have turn penalties, a penalty from file of 0 should produce a neutral effect

    Scenario: Weighting based on turn penalty file, with an extreme negative value -- clamps and does not fail
        Given the turn penalty file
            """
            1,2,5,0,comment
            3,4,7,-30
            """
        And the contract extra arguments "--turn-penalty-file {penalties_file}"
        And the customize extra arguments "--turn-penalty-file {penalties_file}"

        When I route I should get
            | from | to | route    | speed    | time    |
            | a    | e  | ab,be,be | 36 km/h  | 40s +-1 |
            | 1    | e  | ab,be,be | 36 km/h  | 30s +-1 |
            | b    | f  | bc,cf,cf | 36 km/h  | 40s +-1 |
            | 2    | f  | bc,cf,cf | 36 km/h  | 30s +-1 |
            | c    | g  | cd,dg,dg | 72 km/h  | 20s +-1 |
            | 3    | g  | cd,dg,dg | 54 km/h  | 20s +-1 |

    Scenario: Weighting based on turn penalty file with weights
        Given the turn penalty file
            """
            1,2,5,0,-3.33,comment
            3,4,7,-30,100.75
            """
        And the contract extra arguments "--turn-penalty-file {penalties_file}"
        And the customize extra arguments "--turn-penalty-file {penalties_file}"

        When I route I should get
            | from | to | route    | speed    | time    | weights    |
            | a    | e  | ab,be,be | 36 km/h  | 40s +-1 | 16.7,20,0  |
            | 1    | e  | ab,be,be | 36 km/h  | 30s +-1 | 6.8,20,0   |
            | b    | f  | bc,cf,cf | 36 km/h  | 40s +-1 | 20,20,0    |
            | 2    | f  | bc,cf,cf | 36 km/h  | 30s +-1 | 10.1,20,0  |
            | c    | g  | cd,dg,dg | 72 km/h  | 20s +-1 | 120.8,20,0 |
            | 3    | g  | cd,dg,dg | 54 km/h  | 20s +-1 | 110.9,20,0 |
