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

    @traffic
    Scenario: Traffic update on the edge with a traffic signal
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

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1,2,65
        2,1,65
        """
        And the query options
          | annotations | datasources,nodes,speed,duration,weight |

        When I route I should get
          | from | to | route   | speed   | weights | time  | distances | a:datasources | a:nodes | a:speed | a:duration |  a:weight |
          | a    | c  | abc,abc | 59 km/h | 24.2,0  | 24.2s | 399.9m,0m |           1:0 |  1:2:3  |   18:18 |  11.1:11.1 | 11.1:11.1 |
          | c    | a  | abc,abc | 59 km/h | 24.2,0  | 24.2s | 399.9m,0m |           0:1 |  3:2:1  |   18:18 |  11.1:11.1 | 11.1:11.1 |
