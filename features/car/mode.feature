@routing @car @mode
Feature: Car - Mode flag
    Background:
        Given the profile "car"

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
            | from | to | route    | turns                    | modes                 |
            | a    | d  | ab,bc,cd | depart,right,left,arrive | driving,ferry,driving |
            | d    | a  | cd,bc,ab | depart,right,left,arrive | driving,ferry,driving |
            | c    | a  | bc,ab    | depart,left,arrive       | ferry,driving         |
            | d    | b  | cd,bc    | depart,right,arrive      | driving,ferry         |
            | a    | c  | ab,bc    | depart,right,arrive      | driving,ferry         |
            | b    | d  | bc,cd    | depart,left,arrive       | ferry,driving         |

    Scenario: Car - Snapping when using a ferry
        Given the node map
            | a | b |   | c | d |   | e | f |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bcde  |         | ferry | 0:10     |
            | ef    | primary |       |          |

        When I route I should get
            | from | to | route | turns         | modes       | time  |
            | c    | d  | bcde  | depart,arrive | ferry       | 600s  |


