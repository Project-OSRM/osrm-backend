@routing @bicycle @mode
Feature: Bike - Mode flag

    Background:
        Given the profile "bicycle"
        Given a grid size of 5 meters

    Scenario: Bike Sliproad
        Given the node map
            """
                  i
            a b - c-d
                ` |
                g-e-h
                  |
                  |
                  f
            """

        And the nodes
            | node | highway         |
            | c    | traffic_signals |

        And the ways
            | nodes | highway   | name   | oneway:bicycle | maxspeed:forward  |
            | abcd  | cycleway  | street |                | 4 km/h |
            | eb    | path      |        | yes            |  |
            | icef  | tertiary  | road   |                | 4 km/h |
            | geh   | secondary | street |                |  |

        When I route I should get
            | waypoints | route             | turns                               |
            | a,f       | street,,road,road | depart,turn right,turn right,arrive |
