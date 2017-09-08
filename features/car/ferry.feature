@routing @car @ferry
Feature: Car - Handle ferry routes

    Background:
        Given the profile "car"

    Scenario: Car - Use a ferry route
        Given the node map
            """
            a b---c d
            """

        And the ways
            | nodes | highway | route | motorcar |
            | ab    | primary |       |          |
            | bc    |         | ferry | yes      |
            | cd    | primary |       |          |

        When I route I should get
            | from | to | route       | modes                         |
            | a    | c  | ab,bc,bc    | driving,ferry,ferry           |
            | a    | d  | ab,bc,cd,cd | driving,ferry,driving,driving |
            | b    | c  | bc,bc       | ferry,ferry                   |
            | b    | d  | bc,cd,cd    | ferry,driving,driving         |


    Scenario: Car - Ferry with no duration, use default speed
        Given the node map
            """
            a b---c d
            """

        And the ways
            | nodes | highway | route | motorcar |
            | ab    | primary |       |          |
            | bc    |         | ferry | yes      |
            | cd    | primary |       |          |

        When I route I should get
            | from | to | route | modes       | speed  | time |
            | b    | c  | bc,bc | ferry,ferry | 5 km/h | 144s |
            | c    | b  | bc,bc | ferry,ferry | 5 km/h | 144s |

    Scenario: Car - Ferry with simple durations
        Given the node map
            """
            a b---c d
            """

        And the ways
            | nodes | highway | route | motorcar | duration |
            | ab    | primary |       |          |          |
            | bc    |         | ferry | yes      | 00:01:00 |
            | cd    | primary |       |          |          |

        When I route I should get
            | from | to | route | modes       | speed   | time |
            | b    | c  | bc,bc | ferry,ferry | 12 km/h | 60s  |
            | c    | b  | bc,bc | ferry,ferry | 12 km/h | 60s  |

    Scenario: Car - Ferry with ISO 8601 durations
        Given the node map
            """
            a b---c d
            """

        And the ways
            | nodes | highway | route | motorcar | duration |
            | ab    | primary |       |          |          |
            | bc    |         | ferry | yes      | PT1M     |
            | cd    | primary |       |          |          |

        When I route I should get
            | from | to | route | modes       | speed   | time |
            | b    | c  | bc,bc | ferry,ferry | 12 km/h | 60s  |
            | c    | b  | bc,bc | ferry,ferry | 12 km/h | 60s  |

	@snapping
    Scenario: Car - Snapping when using a ferry
        Given the node map
            """
            a
            b-1-2-e 
                  f
            """

        And the ways
            | nodes | highway | route | motorcar | duration |
            | ab    | primary |       |          |          |
            | be  |           | ferry | yes      | 0:30:00  |
            | ef    | primary |       |          |          |

        When I route I should get
            | from | to | route | modes       | time  |
            | 1    | 2  | be,be | ferry,ferry | 600s  |
