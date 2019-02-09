@routing @maxspeed @testbot
Feature: Testbot - Acceleration profiles

    Background: Use specific speeds
        Given the profile "testbot"

    Scenario: Testbot - No stoppage penalties
        Given a grid size of 10 meters
        Given the node map
            """
            a 1 2 3 4 5 b
            """

        And the ways
            | nodes | highway | maxspeed:forward    | maxspeed:backward |
            | ab    | trunk   | 60                  | 45                |

        When I route I should get
            | from | to | route | time   | distance |
            | a    | b  | ab,ab | 3.6s   | 59.9m    |
            | a    | 1  | ab,ab | 0.6s   | 10m      |
            | a    | 2  | ab,ab | 1.2s   | 20m      |
            | a    | 3  | ab,ab | 1.8s   | 30m      |
            | a    | 4  | ab,ab | 2.4s   | 40m      |
            | a    | 5  | ab,ab | 3s     | 50m      |
            | 5    | b  | ab,ab | 0.6s   | 9.9m     |
            | 4    | b  | ab,ab | 1.2s   | 19.9m    |
            | 3    | b  | ab,ab | 1.8s   | 29.9m    |
            | 2    | b  | ab,ab | 2.4s   | 39.9m    |
            | 1    | b  | ab,ab | 3s     | 49.9m    |
            | 1    | 2  | ab,ab | 0.6s   | 10m      |
            | 1    | 3  | ab,ab | 1.2s   | 20m      |
            | 1    | 4  | ab,ab | 1.8s   | 30m      |
            | 1    | 5  | ab,ab | 2.4s   | 40m      |

        When I request a travel time matrix I should get
            |   | a     | 1     | 2   | 3   | 4   | 5   | b   |
            | a | 0     | 0.6   | 1.2 | 1.8 | 2.4 | 3   | 3.6 |
            | 1 | 0.8   | 0     | 0.6 | 1.2 | 1.8 | 2.4 | 3   |
            | 2 | 1.6   | 0.8   | 0   | 0.6 | 1.2 | 1.8 | 2.4 |
            | 3 | 2.4   | 1.6   | 0.8 | 0   | 0.6 | 1.2 | 1.8 |
            | 4 | 3.2   | 2.4   | 1.6 | 0.8 | 0   | 0.6 | 1.2 |
            | 5 | 4     | 3.2   | 2.4 | 1.6 | 0.8 | 0   | 0.6 |
            | b | 4.8   | 4     | 3.2 | 2.4 | 1.6 | 0.8 | 0   |

   Scenario: Testbot - No stoppage points, tiny grid size
        Given a grid size of 1 meters
        Given the node map
            """
            a 1 2 3 4 5 6 7 8 9 b
            """

        And the ways
            | nodes | highway | maxspeed:forward    | maxspeed:backward |
            | ab    | trunk   | 60                  | 45                |

        When I request a travel time matrix I should get
            |   |  a    |  1    |  2   |  3   |  4   |  5    |  6    |  7   |  8   |  9   |  b   |
            | a |  0    |  0    |  0.1 |  0.1 |  0.2 |  0.3  |  0.3  |  0.4 |  0.4 |  0.5 |  0.6 |
            | 1 |  0    |  0    |  0.1 |  0.1 |  0.2 |  0.3  |  0.3  |  0.4 |  0.4 |  0.5 |  0.6 |
            | 2 |  0.1  |  0.1  |  0   |  0   |  0.1 |  0.2  |  0.2  |  0.3 |  0.3 |  0.4 |  0.5 |
            | 3 |  0.2  |  0.2  |  0   |  0   |  0.1 |  0.2  |  0.2  |  0.3 |  0.3 |  0.4 |  0.5 |
            | 4 |  0.3  |  0.3  |  0.2 |  0.1 |  0   |  0.1  |  0.1  |  0.2 |  0.2 |  0.3 |  0.4 |
            | 5 |  0.4  |  0.4  |  0.3 |  0.2 |  0.1 |  0    |  0    |  0.1 |  0.1 |  0.2 |  0.3 |
            | 6 |  0.4  |  0.4  |  0.3 |  0.2 |  0.1 |  0    |  0    |  0.1 |  0.1 |  0.2 |  0.3 |
            | 7 |  0.5  |  0.5  |  0.4 |  0.3 |  0.2 |  0.1  |  0.1  |  0   |  0   |  0.1 |  0.2 |
            | 8 |  0.6  |  0.6  |  0.5 |  0.4 |  0.3 |  0.2  |  0.2  |  0   |  0   |  0.1 |  0.2 |
            | 9 |  0.7  |  0.7  |  0.6 |  0.5 |  0.4 |  0.3  |  0.3  |  0.2 |  0.1 |  0   |  0.1 |
            | b |  0.8  |  0.8  |  0.7 |  0.6 |  0.5 |  0.4  |  0.4  |  0.3 |  0.2 |  0.1 |  0   |

   Scenario: Testbot - No stoppage points, tiny grid size
        Given a grid size of 1 meters
        Given the node map
            """
            a 1 2 3 4 5 6 7 8 9 b
            """

        And the ways
            | nodes | highway | maxspeed:forward    | maxspeed:backward |
            | ab    | trunk   | 60                  | 45                |

        And the query options
            | stoppage_penalty | 1.85,15   |

        When I request a travel time matrix I should get
            |   |  a   |  1   |  2   |  3   |  4   |  5   |  6   |  7   |  8   |  9   |  b   |
            | a |  0   |  1.4 |  2   |  2.5 |  2.9 |  3.2 |  3.6 |  3.8 |  4.1 |  4.4 |  4.6 |
            | 1 |  1.4 |  0   |  1.4 |  2   |  2.5 |  2.9 |  3.2 |  3.6 |  3.8 |  4.1 |  4.4 |
            | 2 |  2   |  1.4 |  0   |  1.4 |  2   |  2.5 |  2.9 |  3.2 |  3.6 |  3.8 |  4.1 |
            | 3 |  2.5 |  2   |  1.4 |  0   |  1.4 |  2   |  2.5 |  2.9 |  3.2 |  3.6 |  3.8 |
            | 4 |  2.9 |  2.5 |  2   |  1.4 |  0   |  1.4 |  2   |  2.5 |  2.9 |  3.2 |  3.6 |
            | 5 |  3.2 |  2.9 |  2.5 |  2   |  1.4 |  0   |  1.4 |  2   |  2.5 |  2.9 |  3.2 |
            | 6 |  3.6 |  3.2 |  2.9 |  2.5 |  2   |  1.4 |  0   |  1.4 |  2   |  2.5 |  2.9 |
            | 7 |  3.8 |  3.6 |  3.2 |  2.9 |  2.5 |  2   |  1.4 |  0   |  1.4 |  2   |  2.5 |
            | 8 |  4.1 |  3.8 |  3.6 |  3.2 |  2.9 |  2.5 |  2   |  1.4 |  0   |  1.4 |  2   |
            | 9 |  4.4 |  4.1 |  3.8 |  3.6 |  3.2 |  2.9 |  2.5 |  2   |  1.4 |  0   |  1.4 |
            | b |  4.6 |  4.4 |  4.1 |  3.8 |  3.6 |  3.2 |  2.9 |  2.5 |  2   |  1.4 |  0   |


    Scenario: Testbot - Use stoppage penalty at waypoints
        Given a grid size of 10 meters
        Given the node map
            """
            a 1 2 3 4 5 b
            """

        And the ways
            | nodes | highway | maxspeed:forward    | maxspeed:backward |
            | ab    | trunk   | 60                  | 45                |

        And the query options
            | stoppage_penalty | 1.85,15   |

        When I request a travel time matrix I should get
            |   | a    | 1    | 2   | 3   | 4   | 5    | b    |
            | a | 0    | 4.6  | 6.5 | 8   | 9.3 | 10.4 | 11.3 |
            | 1 | 4.6  | 0    | 4.6 | 6.5 | 8   | 9.3  | 10.3 |
            | 2 | 6.5  | 4.6  | 0   | 4.6 | 6.5 | 8    | 9.2  |
            | 3 | 8    | 6.5  | 4.6 | 0   | 4.6 | 6.5  | 8    |
            | 4 | 9.3  | 8    | 6.5 | 4.6 | 0   | 4.6  | 6.5  |
            | 5 | 10.4 | 9.3  | 8   | 6.5 | 4.6 | 0    | 4.6  |
            | b | 11.3 | 10.3 | 9.2 | 8   | 6.5 | 4.6  | 0    |


    Scenario: Long distance grid with no penalty
        Given a grid size of 1000 meters
        Given the node map
            """
            a 1 2 3 4 5 b
            """

        And the ways
            | nodes | highway | maxspeed:forward    | maxspeed:backward |
            | ab    | trunk   | 60                  | 45                |

        When I request a travel time matrix I should get
            |   | a     | 1    | 2     | 3     | 4     | 5     | b     |
            | a | 0     | 59.9 | 119.9 | 179.9 | 239.9 | 299.9 | 359.9 |
            | 1 | 79.9  | 0    | 60    | 120   | 180   | 240   | 300   |
            | 2 | 159.9 | 80   | 0     | 60    | 120   | 180   | 240   |
            | 3 | 239.9 | 160  | 80    | 0     | 60    | 120   | 180   |
            | 4 | 319.9 | 240  | 160   | 80    | 0     | 60    | 120   |
            | 5 | 399.9 | 320  | 240   | 160   | 80    | 0     | 60    |
            | b | 479.9 | 400  | 320   | 240   | 160   | 80    | 0     |

    Scenario: Long distance grid
        Given a grid size of 1000 meters
        Given the node map
            """
            a 1 2 3 4 5 b
            """

        And the ways
            | nodes | highway | maxspeed:forward    | maxspeed:backward |
            | ab    | trunk   | 60                  | 45                |

        And the query options
            | stoppage_penalty | 1.85,15   |

        When I request a travel time matrix I should get
            |   | a     | 1     | 2     | 3     | 4     | 5     | b     |
            | a | 0     | 68.9  | 128.9 | 188.9 | 248.9 | 308.9 | 368.9 |
            | 1 | 86.6  | 0     | 69    | 129   | 189   | 249   | 309   |
            | 2 | 166.6 | 86.7  | 0     | 69    | 129   | 189   | 249   |
            | 3 | 246.6 | 166.7 | 86.7  | 0     | 69    | 129   | 189   |
            | 4 | 326.6 | 246.7 | 166.7 | 86.7  | 0     | 69    | 129   |
            | 5 | 406.6 | 326.7 | 246.7 | 166.7 | 86.7  | 0     | 69    |
            | b | 486.6 | 406.7 | 326.7 | 246.7 | 166.7 | 86.7  | 0     |