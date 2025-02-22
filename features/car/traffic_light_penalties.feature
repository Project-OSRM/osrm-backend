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
            | from | to | time   | #                             |
            | 1    | 2  |  11.1s | no turn with no traffic light |
            | 3    | 4  |  13.1s | no turn with traffic light    |
            | g    | j  |  18.7s | turn with no traffic light    |
            | k    | n  |  20.7s | turn with traffic light       |


    Scenario: Car - Traffic signal direction straight
        Given the node map
            """
            a-1-b-2-c

            d-3-e-4-f

            g-5-h-6-i

            j-7-k-8-l

            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | def   | primary |
            | ghi   | primary |
            | jkl   | primary |

        And the nodes
            | node | highway         | traffic_signals:direction |
            | e    | traffic_signals |                           |
            | h    | traffic_signals | forward                   |
            | k    | traffic_signals | backward                  |

        When I route I should get
            | from | to | time   | weight | #                             |
            | 1    | 2  |  11.1s | 11.1   | no turn with no traffic light |
            | 2    | 1  |  11.1s | 11.1   | no turn with no traffic light |
            | 3    | 4  |  13.1s | 13.1   | no turn with traffic light    |
            | 4    | 3  |  13.1s | 13.1   | no turn with traffic light    |
            | 5    | 6  |  13.1s | 13.1   | no turn with traffic light    |
            | 6    | 5  |  11.1s | 11.1   | no turn with no traffic light |
            | 7    | 8  |  11.1s | 11.1   | no turn with no traffic light |
            | 8    | 7  |  13.1s | 13.1   | no turn with traffic light    |


    Scenario: Car - Traffic signal direction with distance weight
        Given the profile file "car" initialized with
        """
        profile.properties.weight_name = 'distance'
        profile.properties.traffic_light_penalty = 100000
        """

        Given the node map
            """
            a---b---c
            1       2
            |       |
            |       |
            |       |
            |       |
            |       |
            d-------f

            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | adfc  | primary |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        When I route I should get
            | from | to | time      | distances | weight | #                                     |
            | 1    | 2  | 100033.2s | 599.9m,0m | 599.8  | goes via the expensive traffic signal |



    Scenario: Car - Encounters a traffic light direction
        Given the node map
            """
              a      f      k      p
              |      |      |      |
            b-c-d  h-g-i  l-m-n  q-r-s
              |      |      |      |
              e      j      o      t

            """

        And the ways
            | nodes | highway |
            | bcd   | primary |
            | ace   | primary |
            | hgi   | primary |
            | fgj   | primary |
            | lmn   | primary |
            | kmo   | primary |
            | qrs   | primary |
            | prt   | primary |

        And the nodes
            | node | highway         | traffic_signals:direction |
            | g    | traffic_signals |                           |
            | m    | traffic_signals | forward                   |
            | r    | traffic_signals | backward                  |


        When I route I should get
            # Base case
            | from | to | time   | #                             |
            | a    | b  | 18.7s  | turn with no traffic light    |
            | a    | e  | 22.2s  | no turn with no traffic light |
            | a    | d  | 21.9s  | turn with no traffic light    |
            | e    | b  | 21.9s  | turn with no traffic light    |
            | e    | a  | 22.2s  | no turn with no traffic light |
            | e    | d  | 18.7s  | turn with no traffic light    |
            | d    | e  | 21.9s  | turn with no traffic light    |
            | d    | b  | 11s    | no turn with no traffic light |
            | d    | a  | 18.7s  | turn with no traffic light    |
            | b    | a  | 21.9s  | turn with no traffic light    |
            | b    | d  | 11s    | no turn with no traffic light |
            | b    | e  | 18.7s  | turn with no traffic light    |
            # All have traffic lights - 2s penalty
            | f    | h  | 20.7s  | turn with traffic light       |
            | f    | j  | 24.2s  | no turn with traffic light    |
            | f    | i  | 23.9s  | turn with traffic light       |
            | j    | h  | 23.9s  | turn with traffic light       |
            | j    | f  | 24.2s  | no turn with traffic light    |
            | j    | i  | 20.7s  | turn with traffic light       |
            | i    | j  | 23.9s  | turn with traffic light       |
            | i    | h  | 13s    | no turn with traffic light    |
            | i    | f  | 20.7s  | turn with traffic light       |
            | h    | f  | 23.9s  | turn with traffic light       |
            | h    | i  | 13s    | no turn with traffic light    |
            | h    | j  | 20.7s  | turn with traffic light       |
            # Front direction have traffic lights - 2s penalty
            | k    | l  | 20.7s  | turn with traffic light       |
            | k    | o  | 24.2s  | no turn with traffic light    |
            | k    | n  | 23.9s  | turn with traffic light       |
            | o    | l  | 21.9s  | turn with no traffic light    |
            | o    | k  | 22.2s  | no turn with no traffic light |
            | o    | n  | 18.7s  | turn with no traffic light    |
            | n    | o  | 21.9s  | turn with no traffic light    |
            | n    | l  | 11s    | no turn with no traffic light |
            | n    | k  | 18.7s  | turn with no traffic light    |
            | l    | k  | 23.9s  | turn with traffic light       |
            | l    | n  | 13s    | no turn with traffic light    |
            | l    | o  | 20.7s  | turn with traffic light       |
            # Reverse direction have traffic lights - 2s penalty
            | p    | q  | 18.7s  | turn with no traffic light    |
            | p    | t  | 22.2s  | no turn with no traffic light |
            | p    | s  | 21.9s  | turn with no traffic light    |
            | t    | q  | 23.9s  | turn with traffic light       |
            | t    | p  | 24.2s  | no turn with traffic light    |
            | t    | s  | 20.7s  | turn with traffic light       |
            | s    | t  | 23.9s  | turn with traffic light       |
            | s    | q  | 13s    | no turn with traffic light    |
            | s    | p  | 20.7s  | turn with traffic light       |
            | q    | p  | 21.9s  | turn with no traffic light    |
            | q    | s  | 11s    | no turn with no traffic light |
            | q    | t  | 18.7s  | turn with no traffic light    |


    Scenario: Traffic Signal Geometry
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
            | a    | c  | abc,abc | _ibE_ibE?gJ?eJ |


    Scenario: Traffic Signal Geometry - forward signal
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
            | node | highway         | traffic_signals:direction |
            | b    | traffic_signals | forward                   |

        When I route I should get
            | from | to | route   | geometry       |
            | a    | c  | abc,abc | _ibE_ibE?gJ?eJ |


    Scenario: Traffic Signal Geometry - reverse signal
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
            | node | highway         | traffic_signals:direction |
            | b    | traffic_signals | reverse                   |

        When I route I should get
            | from | to | route   | geometry       |
            | a    | c  | abc,abc | _ibE_ibE?gJ?eJ |


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
          | a    | c  | abc,abc | 60 km/h | 24.2,0  | 24.2s | 400m,0m   |           1:0 |  1:2:3  |   18:18 |  11.1:11.1 | 11.1:11.1 |
          | c    | a  | abc,abc | 60 km/h | 24.2,0  | 24.2s | 400m,0m   |           0:1 |  3:2:1  |   18:18 |  11.1:11.1 | 11.1:11.1 |


    @traffic
    Scenario: Traffic update on the edge with a traffic signal - forward
        Given the node map
            """
            a - b - c
            """

        And the ways
          | nodes | highway |
          | abc   | primary |


        And the nodes
            | node | highway         | traffic_signals:direction |
            | b    | traffic_signals | forward                   |

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
          | a    | c  | abc,abc | 60 km/h | 24.2,0  | 24.2s | 400m,0m   |           1:0 |  1:2:3  |   18:18 |  11.1:11.1 | 11.1:11.1 |
          | c    | a  | abc,abc | 65 km/h | 22.2,0  | 22.2s | 400m,0m   |           0:1 |  3:2:1  |   18:18 |  11.1:11.1 | 11.1:11.1 |


    @traffic
    Scenario: Traffic update on the edge with a traffic signal - backward
        Given the node map
            """
            a - b - c
            """

        And the ways
          | nodes | highway |
          | abc   | primary |


        And the nodes
            | node | highway         | traffic_signals:direction |
            | b    | traffic_signals | backward                  |

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
          | a    | c  | abc,abc | 65 km/h | 22.2,0  | 22.2s | 400m,0m   |           1:0 |  1:2:3  |   18:18 |  11.1:11.1 | 11.1:11.1 |
          | c    | a  | abc,abc | 60 km/h | 24.2,0  | 24.2s | 400m,0m   |           0:1 |  3:2:1  |   18:18 |  11.1:11.1 | 11.1:11.1 |


    Scenario: Car - Traffic signal straight direction with edge compression
        Given the node map
            """
            a-1-b - c - d-2-e

            """

        And the ways
            | nodes | highway |
            | abcde | primary |

        And the nodes
            | node | highway         | traffic_signals:direction |
            | c    | traffic_signals | forward                   |

        When I route I should get
            | from | to | time   | weight | #                             |
            | 1    | 2  |  35.3s | 35.3   | no turn with traffic light    |
            | 2    | 1  |  33.3s | 33.3   | no turn with no traffic light |


    Scenario: Car - Traffic signal turn direction with edge compression
        Given the node map
            """
                        d
                        |
                        2
                        |
            a-1-b - c - f
                        |
                        e

                        j
                        |
                        4
                        |
            g-3-h - i - k
                        |
                        l

            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | cf    | primary |
            | fd    | primary |
            | fe    | primary |
            | ghi   | primary |
            | ik    | primary |
            | kj    | primary |
            | kl    | primary |

        And the nodes
            | node | highway         | traffic_signals:direction |
            | k    | traffic_signals | forward                   |

        When I route I should get
            | from | to | time   | weight | #                          |
            | 1    | 2  |  44.2s | 44.2   | turn with no traffic light |
            | 2    | 1  |  41s   | 41     | turn with no traffic light |
            | 3    | 4  |  46.2s | 46.2   | turn with traffic light    |
            | 4    | 3  |  41s   | 41     | turn with no traffic light |


    Scenario: Car - Traffic signal turn direction with turn restriction
        Given the node map
            """
                        d
                        |
                        2
                        |
            a-1-b - c - f
                        |
                        e

            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | cf    | primary |
            | fd    | primary |
            | fe    | primary |

        And the nodes
            | node | highway         | traffic_signals:direction |
            | f    | traffic_signals | forward                   |

        And the relations
            | type        | way:from | way:to | way:via  | restriction   |
            | restriction | abc      | fe     | cf       | no_right_turn |

        And the relations
            | type        | way:from | way:to | node:via | restriction     |
            | restriction | df       | fc     | f        | right_turn_only |

        When I route I should get
            | from | to | time   | weight | #                          |
            | 1    | 2  |  46.2s | 46.2   | turn with traffic light    |
            | 2    | 1  |  41s   | 41     | turn with no traffic light |
