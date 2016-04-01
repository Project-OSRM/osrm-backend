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
            | from | to | route       | turns                    | modes                         |
            | a    | d  | ab,bc,cd,cd | depart,right,left,arrive | driving,ferry,driving,driving |
            | d    | a  | cd,bc,ab,ab | depart,right,left,arrive | driving,ferry,driving,driving |
            | c    | a  | bc,ab,ab    | depart,left,arrive       | ferry,driving,driving         |
            | d    | b  | cd,bc,bc    | depart,right,arrive      | driving,ferry,ferry           |
            | a    | c  | ab,bc,bc    | depart,right,arrive      | driving,ferry,ferry           |
            | b    | d  | bc,cd,cd    | depart,left,arrive       | ferry,driving,driving         |

    Scenario: Car - Snapping when using a ferry
        Given the node map
            | a | b |   | c | d |   | e | f |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bcde  |         | ferry | 0:10     |
            | ef    | primary |       |          |

        When I route I should get
            | from | to | route     | turns         | modes       | time  |
            | c    | d  | bcde,bcde | depart,arrive | ferry,ferry | 600s  |


