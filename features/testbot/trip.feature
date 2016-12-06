@trip @testbot
Feature: Basic trip planning

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters

    Scenario: Testbot - Trip: Roundtrip with one waypoint
        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | da    |

        When I plan a trip I should get
            | waypoints | trips  |
            | a         | aa     |

    Scenario: Testbot - Trip: Roundtrip with waypoints (less than 10)
        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | da    |

        When I plan a trip I should get
            | waypoints | trips  | durations |
            | a,b,c,d   | abcda  | 7.6       |
            | d,b,c,a   | dbcad  | 7.6       |

    Scenario: Testbot - Trip: Roundtrip waypoints (more than 10)
        Given the node map
            """
            a b c d
            l     e
            k     f
            j i h g
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |
            | kl    |
            | la    |

        When I plan a trip I should get
            | waypoints               | trips         |
            | a,b,c,d,e,f,g,h,i,j,k,l | alkjihgfedcba |

    Scenario: Testbot - Trip: Roundtrip FS waypoints (more than 10)
        Given the node map
            """
            a b c d
            l     e
            k     f
            j i h g
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |
            | kl    |
            | la    |

        When I plan a trip I should get
            | waypoints               | source | trips         |
            | a,b,c,d,e,f,g,h,i,j,k,l | first  | alkjihgfedcba |

    Scenario: Testbot - Trip: Roundtrip FE waypoints (more than 10)
        Given the query options
            | source | last  |
        Given the node map
            """
            a b c d
            l     e
            k     f
            j i h g
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |
            | kl    |
            | la    |

        When I plan a trip I should get
            | waypoints               | trips         |
            | a,b,c,d,e,f,g,h,i,j,k,l | lkjihgfedcbal |

    Scenario: Testbot - Trip: Unroutable roundtrip with waypoints (less than 10)
        Given the node map
            """
            a b

            d c
            """

         And the ways
            | nodes |
            | ab    |
            | dc    |

         When I plan a trip I should get
            |  waypoints    | status         | message                                       |
            |  a,b,c,d      | NoTrips        | No trip visiting all destinations possible.  |


    Scenario: Testbot - Trip: Unroutable roundtrip with waypoints (more than 10)
        Given the node map
            """
            a b c d
            l     e
            k     f
            j i h g

            q m n
            p o
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |
            | kl    |
            | la    |
            | mn    |
            | no    |
            | op    |
            | pq    |
            | qm    |

        When I plan a trip I should get
            | waypoints                       | status         | message                                      |
            | a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p | NoTrips        | No trip visiting all destinations possible.  |

# Test TFSE
    Scenario: Testbot - Trip: TFSE with errors
        Given the node map
            """
            a b

            d c
            """

         And the ways
            | nodes |
            | ab    |
            | dc    |

         When I plan a trip I should get
            |  waypoints    | source  | destination | roundtrip | status           | message                                       |
            |  a,b,c,d      | first   | last        | false     | NoTrips          | No trip visiting all destinations possible.   |

    Scenario: Testbot - Trip: FSE with waypoints (less than 10)
        Given the node map
            """
            a  b

                  c
            e  d
            """

        And the ways
            | nodes |
            | ab    |
            | ac    |
            | ad    |
            | ae    |
            | bc    |
            | bd    |
            | be    |
            | cd    |
            | ce    |
            | de    |

        When I plan a trip I should get
            |  waypoints  | source | destination |roundtrip | trips  | durations         | distance               |
            |  a,b,d,e,c  | first  | last        | false    | abedc  | 8.200000000000001 | 81.6                   |


    Scenario: Testbot - Trip: FSE with waypoints (more than 10)
        Given the node map
            """
            a b c d e f g h i j k
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |

        When I plan a trip I should get
            |  waypoints              | source | destination | roundtrip |  trips       | durations  | distance  |
            |  a,b,c,d,e,h,i,j,k,g,f  | first  | last        | false     | abcdeghijkf  | 15         | 149.8     |

    Scenario: Testbot - Trip: FSE roundtrip with waypoints (less than 10)
        Given the node map
            """
            a  b

                  c
            e  d
            """

        And the ways
            | nodes |
            | ab    |
            | ac    |
            | ad    |
            | ae    |
            | bc    |
            | bd    |
            | be    |
            | cd    |
            | ce    |
            | de    |

        When I plan a trip I should get
            |  waypoints  | source | destination | roundtrip | trips   |
            |  a,b,d,e,c  | first  | last        | true      | abedca  |


    Scenario: Testbot - Trip: midway points in isoldated roads should return no trips
        Given the node map
            """
            a 1 b

            c 2 d
            """

        And the ways
            | nodes |
            | ab    |
            | cd    |

        When I plan a trip I should get
            | waypoints | status  | message                                     |
            | 1,2       | NoTrips | No trip visiting all destinations possible. |

    Scenario: Testbot - Trip: Repeated Coordinate
        Given the node map
            """
            a   b
            """

        And the ways
            | nodes |
            | ab    |

        When I plan a trip I should get
            | waypoints                                         | trips |
            | a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a |       |


    Scenario: Testbot - Trip: geometry details of geojson
        Given the query options
            | geometries | geojson  |

        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | da    |

        When I plan a trip I should get
            | waypoints | trips  | durations | geometry                                                               |
            | a,b,c,d   | abcda  | 7.6       | 1,1,1.00009,1,1,0.99991,1.00009,1,1,1,1.00009,0.99991,1,1              |
            | d,b,c,a   | dbcad  | 7.6       | 1.00009,0.99991,1,1,1.00009,1,1,0.99991,1.00009,1,1,1,1.00009,0.99991  |

    Scenario: Testbot - Trip: geometry details of polyline
        Given the query options
            | geometries | polyline  |

        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | da    |

        When I plan a trip I should get
            | waypoints | trips  | durations | geometry                                                              |
            | a,b,c,d   | abcda  | 7.6       | 1,1,1,1.00009,0.99991,1,1,1.00009,1,1,0.99991,1.00009,1,1             |
            | d,b,c,a   | dbcad  | 7.6       | 0.99991,1.00009,1,1,1,1.00009,0.99991,1,1,1.00009,1,1,0.99991,1.00009 |

    Scenario: Testbot - Trip: geometry details of polyline6
        Given the query options
            | geometries | polyline6  |

        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cb    |
            | da    |

        When I plan a trip I should get
            | waypoints | trips | durations | geometry                                                              |
            | a,b,c,d   | abcda |       7.6 | 1,1,1,1.00009,0.99991,1,1,1.00009,1,1,0.99991,1.00009,1,1             |
            | d,b,c,a   | dbcad |       7.6 | 0.99991,1.00009,1,1,1,1.00009,0.99991,1,1,1.00009,1,1,0.99991,1.00009 |