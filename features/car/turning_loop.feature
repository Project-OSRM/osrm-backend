@routing @car @turning_loop
Feature: Car - Handle turning loop

    Background:
        Given the profile "car"

    Scenario: Car - Must turn around at the first good opportunity
        Given the node map
            """
            a-b-c-d-e-f-g
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | cd    | primary |
            | de    | primary |
            | ef    | primary |
            | fg    | primary |

        And the nodes
            | node | highway         |
            | b    | turning_loop    |
            | d    | turning_circle  |
            | f    | mini_roundabout |

        When I route I should get
            | waypoints | bearings     | route         | turns                        |
            | a,a       | 90,10 270,10 | ab,ab,ab      | depart,continue uturn,arrive |
            | c,a       | 90,10 270,10 | cd,cd,ab      | depart,continue uturn,arrive |
            | e,a       | 90,10 270,10 | ef,ef,ab      | depart,continue uturn,arrive |
