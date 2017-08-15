@routing  @guidance
Feature: End Of Road Instructions

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: End of Road with through street
        Given the node map
            """
                c
            a e b
              f d
            """

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | cbd    | primary |
            | ef     | primary |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,c       | aeb,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | aeb,cbd,cbd | depart,end of road right,arrive |

    # http://map.project-osrm.org/?z=18&center=38.906632%2C-77.008265&loc=38.906463%2C-77.007621&loc=38.906822%2C-77.008860&hl=en&alt=0
    Scenario: End of Road, unnamed oneway
        Given the node map
            """
                c
            a e b
              f d
            """

        And the ways
            | nodes  | highway | name | oneway |
            | aeb    | primary | road | yes    |
            | cbd    | primary |      | yes    |
            | ef     | primary | turn | yes    |

       When I route I should get
            | waypoints | route  | turns                           |
            | a,d       | road,, | depart,end of road right,arrive |

    @3605
    Scenario: End of Road with oneway through street
        Given the node map
            """
                    c
            a e     b
              f     d
            """

        And the ways
            | nodes  | highway | oneway |
            | aeb    | primary | no     |
            | cbd    | primary | yes    |
            | ef     | primary | no     |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,d       | aeb,cbd,cbd | depart,end of road right,arrive |

    @3605
    Scenario: End of Road fromnameless onto through street
        Given the node map
            """
                    c
            a e     b
              f     d
            """

        And the ways
            | nodes  | highway | oneway | name |
            | aeb    | primary | no     |      |
            | cbd    | primary | yes    | cbd  |
            | ef     | primary | no     | ef   |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,d       | ,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with three streets
        Given the node map
            """
                c
            a e b
              f d
            """

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | cb     | primary |
            | bd     | primary |
            | ef     | primary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | aeb,cb,cb | depart,end of road left,arrive  |
            | a,d       | aeb,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with three streets, slightly angled
        Given the node map
            """
            a e       c
              f       b
                      d
            """

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | cb     | primary |
            | bd     | primary |
            | ef     | primary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | aeb,cb,cb | depart,end of road left,arrive  |
            | a,d       | aeb,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with three streets, slightly angled
        Given the node map
            """
                      c
              f       b
            a e       d
            """

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | ef     | primary |
            | cb     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | aeb,cb,cb | depart,end of road left,arrive  |
            | a,d       | aeb,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with through street, slightly angled
        Given the node map
            """
            a e       c
              f       b
                      d
            """

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | ef     | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,c       | aeb,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | aeb,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with through street, slightly angled
        Given the node map
            """
                      c
              f       b
            a e       d
            """

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | ef     | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,c       | aeb,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | aeb,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with two ramps - prefer ramp over end of road
        Given the node map
            """
                c
            a e b
              f d
            """

        And the ways
            | nodes  | highway       |
            | aeb    | primary       |
            | ef     | primary       |
            | bc     | motorway_link |
            | bd     | motorway_link |

       When I route I should get
            | waypoints | route     | turns                       |
            | a,c       | aeb,bc,bc | depart,on ramp left,arrive  |
            | a,d       | aeb,bd,bd | depart,on ramp right,arrive |

    # http://www.openstreetmap.org/#map=19/52.49907/13.41836
    @end-of-road @negative
    Scenario: Don't Handle Circles as End-Of-Road
        Given the node map
            """
              r       q
                      a s
                  b
                          j

            l   c         i       k

                          h
            m
                d                 n
                  e     g
                    f
                    o   p
            """

        And the ways
            | nodes        | highway     | name  | oneway |
            | abcdefghijsa | secondary   | kotti | yes    |
            | ki           | secondary   | skal  | yes    |
            | cl           | secondary   | skal  | yes    |
            | md           | secondary   | skal  | yes    |
            | gn           | secondary   | skal  | yes    |
            | qa           | tertiary    | adal  | no     |
            | br           | residential | rei   | yes    |
            | fo           | secondary   | kstr  | yes    |
            | pg           | secondary   | kstr  | yes    |

        When I route I should get
            | waypoints | route                | turns                               | #                                                      |
            | k,l       | skal,kotti,skal,skal | depart,turn right,turn right,arrive | # could be a case to find better turn instructions for |
