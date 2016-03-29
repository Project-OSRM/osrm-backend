@routing @car @mode
Feature: Car - Mode flag
    Background:
        Given the profile "car"

    @mokob @2155
    Scenario: Car - Mode when using a ferry
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |

        When I route I should get
            | from | to | route    | turns                       | modes                 |
            | a    | d  | ab,bc,cd | head,right,left,destination | driving,ferry,driving |
            | d    | a  | cd,bc,ab | head,right,left,destination | driving,ferry,driving |
            | c    | a  | bc,ab    | head,left,destination       | ferry,driving         |
            | d    | b  | cd,bc    | head,right,destination      | driving,ferry         |
            | a    | c  | ab,bc    | head,right,destination      | driving,ferry         |
            | b    | d  | bc,cd    | head,left,destination       | ferry,driving         |

    @mokob @2155
    Scenario: Car - Snapping when using a ferry
        Given the node map
            | a | b |   | c | d |   | e | f |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bcde  |         | ferry | 0:10     |
            | ef    | primary |       |          |

        When I route I should get
            | from | to | route | turns            | modes       | time  |
            | c    | d  | bcde  | head,destination | ferry       | 600s  |


