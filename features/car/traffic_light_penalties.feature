@routing @car @traffic_light
Feature: Car - Handle traffic lights

    Background:
        Given the profile "car"

    Scenario: Car - Encounters a traffic light
        Given the node map
            """
            a-1-b-2-c

            d-3-e-4-f

            g-h-i   k-l-m
              |       |
              j       n

            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | def   | primary |
            | ghi   | primary |
            | klm   | primary |
            | hj    | primary |
            | ln    | primary |

        And the nodes
            | node | highway         |
            | e    | traffic_signals |
            | l    | traffic_signals |

        When I route I should get
            | from | to | time   | # |
            | 1    | 2  |  11.1s | no turn with no traffic light |
            | 3    | 4  |  13.1s | no turn with traffic light    |
            | g    | j  |  18.7s | turn with no traffic light    |
            | k    | n  |  20.7s | turn with traffic light       |


    Scenario: Tarrif Signal Geometry
        Given the query options
            | overview   | full      |
            | geometries | polyline  |

        Given the node map
            """
            a - b - c
            """

        And the ways
            | nodes | highway |
            | abc   | primary |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        When I route I should get
            | from | to | route   | geometry       |
            | a    | c  | abc,abc | _ibE_ibE?gJ?gJ |
