@routing @car @mode
Feature: Car - Mode flag
    Background:
        Given the profile "car"

    Scenario: Car - Mode when using a ferry
        Given the node map
            """
            a b
              c d
            """

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |

        When I route I should get
            | from | to | route       | modes                         |
            | a    | d  | ab,bc,cd,cd | driving,ferry,driving,driving |
            | d    | a  | cd,bc,ab,ab | driving,ferry,driving,driving |
            | c    | a  | bc,ab,ab    | ferry,driving,driving         |
            | d    | b  | cd,bc,bc    | driving,ferry,ferry           |
            | a    | c  | ab,bc,bc    | driving,ferry,ferry           |
            | b    | d  | bc,cd,cd    | ferry,driving,driving         |

    Scenario: Car - Snapping when using a ferry
        Given the node map
            """
            a b   c d   e f
            """

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bcde  |         | ferry | 0:10     |
            | ef    | primary |       |          |

        When I route I should get
            | from | to | route     | modes       | time  |
            | c    | d  | bcde,bcde | ferry,ferry | 600s  |


