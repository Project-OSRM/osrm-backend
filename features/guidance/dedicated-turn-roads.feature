@routing  @guidance
Feature: Slipways and Dedicated Turn Lanes

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: Turn Instead of Ramp
        Given the node map
            """
                    e
            a b     c d
                  h

                  1

                    f

                    g
            """

        And the ways
            | nodes | highway    | name   |
            | abcd  | trunk      | first  |
            | bhf   | trunk_link |        |
            | ecfg  | primary    | second |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abcd     | ecfg   | c        | no_right_turn |

       When I route I should get
            | waypoints | route               | turns                           |
            | a,g       | first,second,second | depart,turn right,arrive        |
            | a,1       | first,,             | depart,turn slight right,arrive |

    Scenario: Turn Instead of Ramp
        Given the node map
            """
                    e
            a b     c d
                  h







                    f


                    g
            """

        And the ways
            | nodes | highway       | name   |
            | abcd  | motorway      | first  |
            | bhf   | motorway_link |        |
            | efg   | primary       | second |

       When I route I should get
            | waypoints | route                | turns                                             |
            | a,g       | first,,second,second | depart,off ramp slight right,turn straight,arrive |

    Scenario: Turn Instead of Ramp
        Given the node map
            """
                    e
            a b     c d
                  h



                    f


                    g
            """

        And the ways
            | nodes | highway       | name   |
            | abcd  | motorway      | first  |
            | bhf   | motorway_link |        |
            | efg   | primary       | second |

        When I route I should get
            | waypoints | route                | turns                                             |
            | a,g       | first,,second,second | depart,off ramp slight right,turn straight,arrive |

    Scenario: Inner city expressway with on road
        Given the node map
            """
            a b       c g
                    f



                      d



                      e
            """

        And the ways
            | nodes | highway      | name  |
            | abc   | primary      | road  |
            | cg    | primary      | road  |
            | bfd   | trunk_link   |       |
            | cde   | trunk        | trunk |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cde    | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                    |
            | a,e       | road,trunk,trunk     | depart,turn right,arrive |


    Scenario: Slipway Round U-Turn
        Given the node map
            """
            a   f

            b   e


              g

            c   d
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |

    Scenario: Slipway Steep U-Turn
        Given the node map
            """
            a   f

            b   e
              g


            c   d
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |

    Scenario: Schwarzwaldstrasse Autobahn
        Given the node map
            """
                    i           h         g
                j
            a             k
                  b   r c   d   e         f




                      l
                      m
                        n   q



                        o   p
            """

        And the nodes
            # the traffic light at `l` is not actually in the data, but necessary for the test to check everything
            # http://www.openstreetmap.org/#map=19/48.99211/8.40336
            | node | highway         |
            | r    | traffic_signals |
            | l    | traffic_signals |

        And the ways
            | nodes | highway        | name               | ref  | oneway |
            | abrcd | secondary      | Schwarzwaldstrasse | L561 | yes    |
            | hija  | secondary      | Schwarzwaldstrasse | L561 | yes    |
            | def   | secondary      | Ettlinger Strasse  |      | yes    |
            | gh    | secondary      | Ettlinger Strasse  |      | yes    |
            | blmn  | secondary_link |                    | L561 | yes    |
            | hkc   | secondary_link | Ettlinger Strasse  |      | yes    |
            | qdki  | secondary_link | Ettlinger Allee    |      | yes    |
            | cn    | secondary_link | Ettlinger Allee    |      | yes    |
            | no    | primary        | Ettlinger Allee    |      | yes    |
            | pq    | primary        | Ettlinger Allee    |      | yes    |
            | qe    | secondary_link | Ettlinger Allee    |      | yes    |

        When I route I should get
            | waypoints | route                                                    | turns                    | ref        |
            | a,o       | Schwarzwaldstrasse,Ettlinger Allee,Ettlinger Allee       | depart,turn right,arrive | L561,,     |

    Scenario: Traffic Lights everywhere
        #http://map.project-osrm.org/?z=18&center=48.995336%2C8.383813&loc=48.995467%2C8.384548&loc=48.995115%2C8.382761&hl=en&alt=0
        Given the node map
            """
            a     k l     j
                      d b c i

                        e g

                        1
                          h

                          f
            """

        And the nodes
            | node | highway         |
            | b    | traffic_signals |
            | e    | traffic_signals |
            | g    | traffic_signals |

        And the ways
            | nodes  | highway        | name          | oneway |
            | aklbci | secondary      | Ebertstrasse  | yes    |
            | kdeh   | secondary_link |               | yes    |
            | jcghf  | primary        | Brauerstrasse | yes    |

        When I route I should get
            | waypoints | route                                    | turns                    |
            | a,i       | Ebertstrasse,Ebertstrasse                | depart,arrive            |
            | a,l       | Ebertstrasse,Ebertstrasse                | depart,arrive            |
            | a,f       | Ebertstrasse,Brauerstrasse,Brauerstrasse | depart,turn right,arrive |
            | a,1       | Ebertstrasse,,                           | depart,turn right,arrive |

    #2839
    Scenario: Self-Loop
        Given the node map
            """
                                                      l     k
                                                                  j
                                                  m
                                                                      i


                                                                        h

                                                n

                                                                        g
                                                o

                                                                      f
                                              p
                                                                  e
            a         b                 c                   d
            """

     And the ways
            | nodes           | name    | oneway | highway     | lanes |
            | abc             | circled | no     | residential | 1     |
            | cdefghijklmnopc | circled | yes    | residential | 1     |

     When I route I should get
            | waypoints | bearings     | route           | turns         |
            | b,a       | 90,10 270,10 | circled,circled | depart,arrive |

    @todo
    #due to the current turn function, the left turn bcp is exactly the same cost as pcb, a right turn. The right turn should be way faster,though
    #for that reason we cannot distinguish between driving clockwise through the circle or counter-clockwise. Until this is improved, this case here
    #has to remain as todo (see #https://github.com/Project-OSRM/osrm-backend/pull/2849)
    Scenario: Self-Loop - Bidirectional
        Given the node map
            """
                                                      l     k
                                                                  j
                                                  m
                                                                      i


                                                                        h

                                                n

                                                                        g
                                                o

                                                                      f
                                              p
                                                                  e
            a         b                 c                   d
            """

     And the ways
            | nodes           | name    | oneway | highway     | lanes |
            | abc             | circled | no     | residential | 1     |
            | cdefghijklmnopc | circled | no     | residential | 1     |

     When I route I should get
            | waypoints | bearings     | route           | turns         |
            | b,a       | 90,10 270,10 | circled,circled | depart,arrive |

    #http://www.openstreetmap.org/#map=19/38.90597/-77.01276
    Scenario: Don't falsly classify as sliproads
        Given the node map
            """
                                                          j
            a b                                           c             d





                    e














                                                          1

                                                f         g

                                                          i             h
            """

        And the ways
            | nodes | name       | highway   | oneway | maxspeed |
            | abcd  | new york   | primary   | yes    | 35       |
            | befgh | m street   | secondary | yes    | 35       |
            | igcj  | 1st street | tertiary  | no     | 20       |

        And the nodes
            | node | highway         |
            | c    | traffic_signals |
            | g    | traffic_signals |

        When I route I should get
            | waypoints | route                                   | turns                              | #                                    |
            | a,d       | new york,new york                       | depart,arrive                      | this is the sinatra route            |
            | a,j       | new york,1st street,1st street          | depart,turn left,arrive            |                                      |
            | a,1       | new york,m street,1st street,1st street | depart,turn right,turn left,arrive | this can false be seen as a sliproad |

    # Merging into degree two loop on dedicated turn detection / 2927
    Scenario: Turn Instead of Ramp
        Given the node map
            """
                                          f
                    g           h
                                    d     e
            i       c           j








                b

                a
            """

        And the ways
            | nodes | highway | name | oneway |
            | abi   | primary | road | yes    |
            | bcjd  | primary | loop | yes    |
            | dhgf  | primary | loop | yes    |
            | fed   | primary | loop | yes    |

        And the nodes
            | node | highway         |
            | g    | traffic_signals |
            | c    | traffic_signals |

        # We don't actually care about routes here, this is all about endless loops in turn discovery
        When I route I should get
            | waypoints | route          | turns                          |
            | a,i       | road,road,road | depart,fork slight left,arrive |
