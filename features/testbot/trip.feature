@trip @testbot
Feature: Basic trip planning

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters

    Scenario: Testbot - Trip: Invalid options (like was in test suite for a long time)
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
            | waypoints   | trips  | code           |
            | a           |        | InvalidOptions |

    Scenario: Testbot - Trip: Roundtrip between same waypoint
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
            | waypoints   | trips  | code |
            | a,a         | aa     | Ok   |

    Scenario: Testbot - Trip: data version check
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

        And the extract extra arguments "--data_version cucumber_data_version"

        When I plan a trip I should get
            | waypoints   | trips  | data_version          | code |
            | a,a         | aa     | cucumber_data_version | Ok   |

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
            | waypoints | trips  | durations | code |
            | a,b,c,d   | abcda  | 7.6       | Ok   |
            | d,b,c,a   | dbcad  | 7.6       | Ok   |

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

    Scenario: Testbot - Trip: FS waypoints (less than 10)
        Given the query options
            | source | first  |
        Given the node map
            """
            a b c d
            l     e

            j i   g
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | de    |
            | eg    |
            | gi    |
            | ij    |
            | jl    |
            | la    |

        When I plan a trip I should get
            | waypoints               | trips         | roundtrip | durations |
            | a,b,c,d,e,g,i,j,l       | abcdegijla    | true      | 22        |
            | a,b,c,d,e,g,i,j,l       | abcljiged     | false     | 13        |


    Scenario: Testbot - Trip: FS waypoints (more than 10)
        Given the query options
            | source | first  |
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
            | waypoints               | trips         | roundtrip | durations |
            | a,b,c,d,e,f,g,h,i,j,k,l | alkjihgfedcba | true      | 22        |
            | a,b,c,d,e,f,g,h,i,j,k,l | acblkjihgfed  | false     | 13        |


    Scenario: Testbot - Trip: FE waypoints (less than 10)
        Given the query options
            | destination | last  |
        Given the node map
            """
            a b c d
            l     e

            j i   g
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | de    |
            | eg    |
            | gi    |
            | ij    |
            | jl    |
            | la    |

        When I plan a trip I should get
            | waypoints             | trips        | roundtrip | durations |
            | a,b,c,d,e,g,i,j,l     | labcdegijl   | true      | 22        |
            | a,b,c,d,e,g,i,j,l     | degijabcl    | false     | 14        |

    Scenario: Testbot - Trip: FE waypoints (more than 10)
        Given the query options
            | destination | last  |
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
            | waypoints               | trips         | roundtrip | durations |
            | a,b,c,d,e,f,g,h,i,j,k,l | lkjihgfedcbal | true      | 22        |
            | a,b,c,d,e,f,g,h,i,j,k,l | cbakjihgfedl  | false     | 19        |

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
            |  a,b,d,e,c  | first  | last        | false    | abedc  | 8.200000000000001 | 81.4                   |


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


    Scenario: Testbot - Trip: midway points in isolated roads should return no trips
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
