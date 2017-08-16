@routing  @guidance @obvious
Feature: Simple Turns

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    #https://www.openstreetmap.org/#map=19/52.51802/13.31337
    Scenario: Crossing a square
        Given the node map
            """
                             d
                             |
            e - - - - - - -  c - g
            |                |
            |                |
            |               |
            |               |
            f - - - - - - - b
                           |
                          a
            """

        And the ways
            | nodes | highway     | name  | oneway |
            | ab    | residential | losch | no     |
            | cd    | residential | ron   | no     |
            | cefb  | residential | alt   | yes    |
            | bc    | residential | alt   | no     |
            | gc    | residential | guer  | no     |

        When I route I should get
            | from | to | route     | turns         |
            | a    | d  | losch,ron | depart,arrive |

    #https://www.openstreetmap.org/#map=18/52.51355/13.21988
    Scenario: Turning tertiary next to residential
        Given the node map
            """
            a - - - b - - - - c
                    |
                     d
                      ` _
                          `-_
                              e
            """

         And the ways
            | nodes | highway     | name  |
            | abde  | tertiary    | havel |
            | bc    | residential | anger |

         When I route I should get
            | from | to | route             | turns                        |
            | a    | c  | havel,anger       | depart,arrive                |
            | a    | e  | havel,havel,havel | depart,continue right,arrive |

     #https://www.openstreetmap.org/#map=19/52.50996/13.23183
     Scenario: Tertiary turning at unclassified
        Given the node map
            """
            a - - - b - - - c
                    |
                    |
                    d
            """

        And the ways
            | nodes | highway      |
            | ab    | tertiary     |
            | bd    | tertiary     |
            | bc    | unclassified |

        When I route I should get
            | from | to | route    | turns                    |
            | a    | c  | ab,bc    | depart,arrive            |
            | a    | d  | ab,bd,bd | depart,turn right,arrive |

    #https://www.openstreetmap.org/#map=19/52.50602/13.25468
    Scenario: Small offset due to large Intersection
        Given the node map
            """
            a - - - - - b - - - - - - c
                        |
                        d
                     `     `
                  `           `
               f                 e
            """

        And the ways
            | nodes | highway     |
            | abc   | residential |
            | bde   | residential |
            | df    | residential |

        When I route I should get
            | from | to | route       | turns                          |
            | a    | e  | abc,bde,bde | depart,turn right,arrive       |
            | a    | f  | abc,df,df   | depart,turn sharp right,arrice |

    # https://www.openstreetmap.org/#map=19/52.49709/13.26620
    # https://www.openstreetmap.org/#map=19/52.49458/13.26273
    Scenario: Offsets in road
        Given the node map
        """
                                              i
                                              |
                                              |
                                              |
        a - - - - - - - b                     e - - - - - - - - - f
                          `c - - - - - - - d`
                           |               |
                           |               |
                           |               |
                           g               h
        """

        And the ways
            | nodes  | highway     | name   |
            | abcdef | residential | Zikade |
            | gc     | residential | Lärche |
            | hd     | residential | Kiefer |
            | ei     | residential | Kiefer |

        When I route I should get
            | from | to | route                | turns                    | locations |
            | a    | f  | Zikade,Zikade        | depart,arrive            | a,f       |
            | f    | a  | Zikade,Zikade        | depart,arrive            | f,a       |
            | a    | g  | Zikade,Lärche,Lärche | depart,turn right,arrive | a,c,g     |
            | h    | i  | Kiefer,Kiefer        | depart,arrive            | h,i       |
            | i    | h  | Kiefer,Kiefer        | depart,arrive            | i,h       |
            | h    | f  | Kiefer,Zikade,Zikade | depart,turn right,arrive | h,d,f     |


     # https://www.openstreetmap.org/#map=20/52.49408/13.27375
     Scenario: Straight on unnamed service
        Given the node map
            """
            a - - - - b - - - - c
                         ` .
                             d
            """

        And the ways
            | nodes | highway | name |
            | abc   | service |      |
            | bd    | service |      |

        When I route I should get
            | from | to | route | turns                           | locations |
            | a    | c  | ,     | depart,arrive                   | a,c       |
            | a    | d  | ,,    | depart,turn slight right,arrive | a,b,d     |

    # https://www.openstreetmap.org/#map=19/52.49198/13.28069
    Scenario: Curved roads at turn
        Given the node map
            """
                               ............g
                            .f
                        e```
                       /
            a  - - - - b
                         c
                           `
                            `
                             `
                               d
            """

        And the ways
            | nodes | highway     | name    |
            | abcd  | residential | herbert |
            | befg  | residential | casper  |

        When I route I should get
            | from | to | route                 | turns                   |
            | a    | d  | herbert,herbert       | depart,arrive           |
            | a    | g  | herbert,casper,casper | depart,turn left,arrive |

    # https://www.openstreetmap.org/#map=19/52.49189/13.28431
    Scenario: Turning residential
        Given the node map
            """
                      d
                     `
            a - - - b
                      \
                        \
                          c
            """

        And the ways
            | nodes | highway     | name    | oneway |
            | abc   | residential | bismark | yes    |
            | bd    | residential | caspar  | yes    |

        When I route I should get
            | from | to | route                 | turns                   |
            | a    | c  | bismark,bismark       | depart,arrive           |
            | a    | d  | bismark,caspar,caspar | depart,turn left,arrive |

    # https://www.openstreetmap.org/#map=19/52.48681/13.28547
    Scenario: Slight Loss in Category with turning road
        Given the node map
            """
                             g
                            /
                           f
                         e   ..c - - - - d
            a - - - - b`````
            """

        And the ways
            | nodes | highway     | name |
            | ab    | tertiary    | warm |
            | bcd   | residential | warm |
            | begg  | tertiary    | paul |

        When I route I should get
            | from | to | route          | turns                          |
            | a    | d  | warm,warm      | depart,arrive                  |
            | a    | g  | warm,paul,paul | depart,turn slight left,arrive |

    # https://www.openstreetmap.org/#map=19/52.48820/13.29947
    Scenario: Driveway within curved road
        Given the node map
            """
            f--e
                 \
                  `
               g   d
                \  |
                  \|
                   c
                 ./
                .b
            a-`
            """

        And the ways
            | nodes | highway     | name    | oneway |
            | abc   | residential | charlot | no     |
            | cdef  | residential | fried   | yes    |
            | gc    | service     |         |        |

        When I route I should get
            | from | to | route         | turns                   |
            | a    | f  | charlot,fried | depart,arrive           |
            | a    | g  | charlot,,     | depart,turn left,arrive |

    # https://www.openstreetmap.org/map=20/52.46815/13.33984
    Scenario: Curve onto end of the road
        Given the node map
            """
            d - - - - e - f-_
                             ``g
                                 h
                    a - - - - - - b - - _ _ _
                                              ` ` ` c
            """

        And the ways
            | nodes   | highway     | name |
            | ab      | residential | menz |
            | defghbc | residential | rem  |

        When I route I should get
            | from | to | route       | turns                        |
            | a    | c  | menz,rem    | depart,arrive                |
            | d    | c  | rem,rem,rem | depart,continue left,arrive  |
            | c    | d  | rem,rem,rem | depart,continue right,arrive |
            | c    | a  | rem,menz    | depart,arrive                |

    # https://www.openstreetmap.org/#map=19/37.58151/-122.34863
    Scenario: Straight towards oneway street, Service Category, Unnamed
        Given the node map
            """
            a - - b - - c
                  |
                  d
            """

        And the ways
            | nodes | highway | name | oneway |
            | ab    | service |      |        |
            | cb    | service |      | yes    |
            | bd    | service |      |        |

        When I route I should get
            | from | to | route | turns                    |
            | a    | d  | ,     | depart,turn right,arrive |
