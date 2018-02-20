@routing  @guidance
Feature: Motorway Guidance

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Ramp Exit Right
        Given the node map
            """
            a-b-c-d-e
               `--f-g
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | bfg   | motorway_link | yes    |

       When I route I should get
            | waypoints | route         | turns                                |
            | a,e       | abcde,abcde   | depart,arrive                        |
            | a,g       | abcde,bfg,bfg | depart,off ramp slight right,arrive |

    Scenario: Ramp Exit Right Curved Right
        Given the node map
            """
            a-b-c
               `f`d
                 `g`e
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | bfg   | motorway_link | yes    |

       When I route I should get
            | waypoints | route         | turns                         |
            | a,e       | abcde,abcde   | depart,arrive                 |
            | a,g       | abcde,bfg,bfg | depart,off ramp right,arrive |

    Scenario: Ramp Exit Right Curved Left
        Given the node map
            """
                   ,e
                 ,d,g
            a-b-c-f
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | cfg   | motorway_link | yes    |

       When I route I should get
            | waypoints | route         | turns                                |
            | a,e       | abcde,abcde   | depart,arrive                        |
            | a,g       | abcde,cfg,cfg | depart,off ramp slight right,arrive |


    Scenario: Ramp Exit Left
        Given the node map
            """
               /--f-g
            a-b-c-d-e
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | bfg   | motorway_link | yes    |

       When I route I should get
            | waypoints | route         | turns                               |
            | a,e       | abcde,abcde   | depart,arrive                       |
            | a,g       | abcde,bfg,bfg | depart,off ramp slight left,arrive |

    Scenario: Ramp Exit Left Curved Left
        Given the node map
            """
                 ,g,e
               ,f,d
            a-b-c
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | bfg   | motorway_link | yes    |

       When I route I should get
            | waypoints | route         | turns                        |
            | a,e       | abcde,abcde   | depart,arrive                |
            | a,g       | abcde,bfg,bfg | depart,off ramp left,arrive |

    Scenario: Ramp Exit Left Curved Right
        Given the node map
            """
            a-b-c-f
                 `d`g
                   `e
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | cfg   | motorway_link | yes    |

       When I route I should get
            | waypoints | route         | turns                               |
            | a,e       | abcde,abcde   | depart,arrive                       |
            | a,g       | abcde,cfg,cfg | depart,off ramp slight left,arrive |

    Scenario: On Ramp Right
        Given the node map
            """
            a-b-c-d-e
            f-g---'
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | fgd   | motorway_link | yes    |

       When I route I should get
            | waypoints | route           | turns                           |
            | a,e       | abcde,abcde     | depart,arrive                   |
            | f,e       | fgd,abcde,abcde | depart,merge slight left,arrive |

    Scenario: On Ramp Left
        Given the node map
            """
            f-g---,
            a-b-c-d-e
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | fgd   | motorway_link | yes    |

       When I route I should get
            | waypoints | route           | turns                            |
            | a,e       | abcde,abcde     | depart,arrive                    |
            | f,e       | fgd,abcde,abcde | depart,merge slight right,arrive |

    Scenario: Highway Fork
        Given the node map
            """
                 /--d-e
            a-b-c
                 \--f-g
            """

        And the ways
            | nodes  | highway  |
            | abcde  | motorway |
            | cfg    | motorway |

       When I route I should get
            | waypoints | route             | turns                           |
            | a,e       | abcde,abcde,abcde | depart,fork slight left,arrive  |
            | a,g       | abcde,cfg,cfg     | depart,fork slight right,arrive |

     Scenario: Fork After Ramp
       Given the node map
            """
                 /--d-e
            a-b-c
                 \--f-g
            """

        And the ways
            | nodes | highway       | oneway |
            | abc   | motorway_link | yes    |
            | cde   | motorway      |        |
            | cfg   | motorway      |        |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,e       | abc,cde,cde | depart,fork slight left,arrive  |
            | a,g       | abc,cfg,cfg | depart,fork slight right,arrive |

     Scenario: On And Off Ramp Right
       Given the node map
            """
            a-b---c---d-e
            f-g--/ \--h i
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | fgc   | motorway_link | yes    |
            | chi   | motorway_link | yes    |

       When I route I should get
            | waypoints | route           | turns                              |
            | a,e       | abcde,abcde     | depart,arrive                      |
            | f,e       | fgc,abcde,abcde | depart,merge slight left,arrive    |
            | a,i       | abcde,chi,chi   | depart,off ramp slight right,arrive |
            | f,i       | fgc,chi,chi     | depart,off ramp slight right,arrive |

    Scenario: On And Off Ramp Left
       Given the node map
            """
            f-g--\/---h-i
            a-b---c---d-e
            """

        And the ways
            | nodes | highway       | oneway |
            | abcde | motorway      |        |
            | fgc   | motorway_link | yes    |
            | chi   | motorway_link | yes    |

       When I route I should get
            | waypoints | route           | turns                             |
            | a,e       | abcde,abcde     | depart,arrive                     |
            | f,e       | fgc,abcde,abcde | depart,merge slight right,arrive  |
            | a,i       | abcde,chi,chi   | depart,off ramp slight left,arrive |
            | f,i       | fgc,chi,chi     | depart,off ramp slight left,arrive |

    Scenario: Merging Motorways
        Given the node map
            """
            e\
            a-b-c
            d/
            """

        And the ways
            | nodes | highway  |
            | abc   | motorway |
            | db    | motorway |
            | eb    | motorway |

        When I route I should get
            | waypoints | route      | turns                            |
            | d,c       | db,abc,abc | depart,merge slight left,arrive  |
            | e,c       | eb,abc,abc | depart,merge slight right,arrive |

    Scenario: Handle 90 degree off ramps correctly
        Given the node map
            """
            a\
            x-b---c-y
                  d
            """

        And the ways
            | nodes | name | highway       | oneway |
            | ab    | On   | motorway_link | yes    |
            | xb    | Hwy  | motorway      |        |
            | bc    | Hwy  | motorway      |        |
            | cd    | Off  | motorway_link | yes    |
            | cy    | Hwy  | motorway      |        |

       When I route I should get
            | waypoints | route               | turns                                           |
            | a,d       | On,Hwy,Off,Off      | depart,merge slight right,off ramp right,arrive |

    #http://0.0.0.0:9966/?z=18&center=38.893323%2C-77.055117&loc=38.893551%2C-77.054833&loc=38.893112%2C-77.055536&hl=en&alt=0
    Scenario: Merging with same name
        Given the node map
            """
            a - - -
                    > c - d
                 b
            """

        And the ways
            | nodes | name | ref         | highway  | oneway |
            | ac    |      | US 50       | motorway | yes    |
            | bc    |      | I 66        | motorway | yes    |
            | cd    |      | US 50; I 66 | motorway | yes    |

        When I route I should get
            | waypoints | route | turns         |
            | a,d       | ,     | depart,arrive |
            | b,d       | ,     | depart,arrive |


    Scenario: Ramp Exit with Lower Priority
        Given the node map
            """
            a-b-c-d-e
               `--f-g
            """

        And the ways
            | nodes | highway      | oneway |
            | abcde | trunk        |        |
            | bfg   | primary_link | yes    |

       When I route I should get
            | waypoints | route         | turns                               |
            | a,e       | abcde,abcde   | depart,arrive                       |
            | a,g       | abcde,bfg,bfg | depart,off ramp slight right,arrive |


    # https://www.openstreetmap.org/node/67366428#map=18/33.64613/-84.44425
    Scenario: Ramp Bifurcations should not be suppressed
        Given the node map
            """
                 /-----------c      /-----------e
            a---b------------------d------------f
            """

        And the ways
            | nodes | highway       | name | destination                                         |
            | ab    | motorway      |      |                                                     |
            | bc    | motorway_link |      | City 17                                             |
            | bd    | motorway_link |      |                                                     |
            | de    | motorway_link |      | Domestic Terminal;Camp Creek Parkway;Riverdale Road |
            | df    | motorway_link |      | Montgomery                                          |


       When I route I should get
            | waypoints | route | turns                                            |
            | a,c       | ,,    | depart,fork slight left,arrive                   |
            | a,e       | ,,,   | depart,fork slight right,fork slight left,arrive  |
            | a,f       | ,,,   | depart,fork slight right,fork slight right,arrive |


    # https://www.openstreetmap.org/#map=19/53.46186/-2.24509
    Scenario: Highway Fork with a Link
        Given the node map
            """
                 /-----------d
            a-b-c------------e
                 \-----------f
            """

        And the ways
            | nodes | highway       |
            | abce  | motorway      |
            | cf    | motorway      |
            | cd    | motorway_link |

       When I route I should get
            | waypoints | route      | turns                              |
            | a,d       | abce,cd,cd | depart,off ramp slight left,arrive |
            | a,e       | abce,abce  | depart,arrive                      |
            | a,f       | abce,cf,cf | depart,turn slight right,arrive |
