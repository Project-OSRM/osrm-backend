@trip @testbot
Feature: Basic trip planning

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters

    Scenario: Testbot - Trip planning with less than 10 waypoints
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

    Scenario: Testbot - Trip planning with more than 10 waypoints
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


    Scenario: Testbot - Trip planning with less than 10 waypoints tfse
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
            |  waypoints  | source | destination | trips  | durations         | distance |
            |  a,b,c,d,e  | 4      | 4           | abcdea | 10.3              | 103      |
            |  a,b,c,d,e  | 0      | 2           | abedc  | 8.200000000000001 | 81.6     |


    Scenario: Testbot - Trip planning with more than 10 nodes tfse
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
            |  waypoints              | source | destination | trips        | durations  | distance  |
            |  a,b,c,d,e,f,g,h,i,j,k  | 0      | 10          | abcdefghijk  | 10         | 99.9      |
            |  a,b,c,d,e,f,g,h,i,j,k  | 4      | 4           | abcdefghijka | 20         | 199.9     |



    Scenario: Testbot - Trip planning with multiple scc roundtrip
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
            | waypoints                       | trips               |
            | a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p | abcdefghijkla,mnopm |


    Scenario: Testbot - Trip planning with fixed start and end points errors
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
            |  waypoints    | source  | destination | status         | message                                       |
            |  a,b,c,d      | 0       | 3           | NoTrips        | No route possible from source to destination  |
            |  a,b,c,d      | -1      | 1           | InvalidQuery   | Query string malformed close to position 123  |
            |  a,b,c,d      | 1       | -1          | InvalidQuery   | Query string malformed close to position 137  |
            |  a,b,c,d      | 10      | 1           | InvalidValue   | Invalid source or destination value.          |
            |  a,b,c,d      | 1       | 10          | InvalidValue   | Invalid source or destination value.          |


    # Test single node in each component #1850
    Scenario: Testbot - Trip planning with less than 10 nodes
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
            | waypoints | trips |
            | 1,2       |       |

    Scenario: Testbot - Repeated Coordinate
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


    Scenario: Testbot - Trip with geometry details of geojson
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

    Scenario: Testbot - Trip with geometry details of polyline
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

    Scenario: Testbot - Trip with geometry details of polyline6
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
