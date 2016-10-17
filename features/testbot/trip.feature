@trip @testbot
Feature: Basic trip planning

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters

    Scenario: Testbot - Trip planning with less than 10 nodes
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
            | waypoints | trips | durations |
            | a,b,c,d   | abcd  | 7.6       |
            | d,b,c,a   | dbca  | 7.6       |

    Scenario: Testbot - Trip planning with more than 10 nodes
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


        When I plan a trip I should get
            | waypoints               | trips         |
            | a,b,c,d,e,f,g,h,i,j,k,l | cbalkjihgfedc |

    Scenario: Testbot - Trip planning with multiple scc
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
            | waypoints                       | trips              |
            | a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p | cbalkjihgfedc,mnop |

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

