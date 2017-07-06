@routing @foot @startpoint
Feature: Foot - Allowed start/end modes

    Background:
        Given the profile "foot"

    Scenario: Foot - Don't start/stop on ferries
        Given the node map
            """
            a 1 b 2 c
            """

        And the ways
            | nodes | highway | route | bicycle |
            | ab    | primary |       |         |
            | bc    |         | ferry | yes     |

        When I route I should get
            | from | to | route | modes           |
            | 1    | 2  | ab,ab | walking,walking |
            | 2    | 1  | ab,ab | walking,walking |

    Scenario: Foot - Don't start/stop on trains
        Given the node map
            """
            a 1 b 2 c
            """

        And the ways
            | nodes | highway | railway | bicycle |
            | ab    | primary |         |         |
            | bc    |         | train   | yes     |

        When I route I should get
            | from | to | route | modes           |
            | 1    | 2  | ab,ab | walking,walking |
            | 2    | 1  | ab,ab | walking,walking |
