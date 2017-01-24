@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "car"
        Given a grid size of 3 meters

    Scenario: Enter and Exit
        Given the node map
            """
                a
                b
            h g   c d
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bgecb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                                         |
           | a,d       | ab,cd,cd | depart,roundabout turn left exit-3,arrive     |
           | a,f       | ab,ef,ef | depart,roundabout turn straight exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout turn right exit-1,arrive    |
           | d,f       | cd,ef,ef | depart,roundabout turn left exit-3,arrive     |
           | d,h       | cd,gh,gh | depart,roundabout turn straight exit-2,arrive |
           | d,a       | cd,ab,ab | depart,roundabout turn right exit-1,arrive    |
           | f,h       | ef,gh,gh | depart,roundabout turn left exit-3,arrive     |
           | f,a       | ef,ab,ab | depart,roundabout turn straight exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout turn right exit-1,arrive    |
           | h,a       | gh,ab,ab | depart,roundabout turn left exit-3,arrive     |
           | h,d       | gh,cd,cd | depart,roundabout turn straight exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout turn right exit-1,arrive    |

    Scenario: Enter and Exit - Rotated
        Given the node map
            """
            a     d
              b c
              g e
            h     f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bgecb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                                         |
           | a,d       | ab,cd,cd | depart,roundabout turn left exit-3,arrive     |
           | a,f       | ab,ef,ef | depart,roundabout turn straight exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout turn right exit-1,arrive    |
           | d,f       | cd,ef,ef | depart,roundabout turn left exit-3,arrive     |
           | d,h       | cd,gh,gh | depart,roundabout turn straight exit-2,arrive |
           | d,a       | cd,ab,ab | depart,roundabout turn right exit-1,arrive    |
           | f,h       | ef,gh,gh | depart,roundabout turn left exit-3,arrive     |
           | f,a       | ef,ab,ab | depart,roundabout turn straight exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout turn right exit-1,arrive    |
           | h,a       | gh,ab,ab | depart,roundabout turn left exit-3,arrive     |
           | h,d       | gh,cd,cd | depart,roundabout turn straight exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout turn right exit-1,arrive    |

    Scenario: Only Enter
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route          | turns                                   |
           | a,c       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | a,e       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | a,g       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,e       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,g       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,b       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,g       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,b       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,c       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,b       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,c       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,e       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |

    Scenario: Only Exit
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route       | turns                           |
           | b,d       | bcegb,cd,cd | depart,roundabout-exit-1,arrive |
           | b,f       | bcegb,ef,ef | depart,roundabout-exit-2,arrive |
           | b,h       | bcegb,gh,gh | depart,roundabout-exit-3,arrive |
           | c,f       | bcegb,ef,ef | depart,roundabout-exit-1,arrive |
           | c,h       | bcegb,gh,gh | depart,roundabout-exit-2,arrive |
           | c,a       | bcegb,ab,ab | depart,roundabout-exit-3,arrive |
           | e,h       | bcegb,gh,gh | depart,roundabout-exit-1,arrive |
           | e,a       | bcegb,ab,ab | depart,roundabout-exit-2,arrive |
           | e,d       | bcegb,cd,cd | depart,roundabout-exit-3,arrive |
           | g,a       | bcegb,ab,ab | depart,roundabout-exit-1,arrive |
           | g,d       | bcegb,cd,cd | depart,roundabout-exit-2,arrive |
           | g,f       | bcegb,ef,ef | depart,roundabout-exit-3,arrive |
      #phantom node snapping can result in a full round-trip here, therefore we cannot test b->a and the other direct exits

    Scenario: Drive Around
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route       | turns         |
           | b,c       | bcegb,bcegb | depart,arrive |
           | b,e       | bcegb,bcegb | depart,arrive |
           | b,g       | bcegb,bcegb | depart,arrive |
           | c,e       | bcegb,bcegb | depart,arrive |
           | c,g       | bcegb,bcegb | depart,arrive |
           | c,b       | bcegb,bcegb | depart,arrive |
           | e,g       | bcegb,bcegb | depart,arrive |
           | e,b       | bcegb,bcegb | depart,arrive |
           | e,c       | bcegb,bcegb | depart,arrive |
           | g,b       | bcegb,bcegb | depart,arrive |
           | g,c       | bcegb,bcegb | depart,arrive |
           | g,e       | bcegb,bcegb | depart,arrive |

     Scenario: Mixed Entry and Exit - Not an Intersection
        Given the node map
           """
             c   a
           j   b   f
             k   e
           l   h   d
             g   i
           """

        And the ways
           | nodes | junction   | oneway |
           | abc   |            | yes    |
           | def   |            | yes    |
           | ghi   |            | yes    |
           | jkl   |            | yes    |
           | bkheb | roundabout | yes    |

        When I route I should get
           | waypoints | route       | turns                           |
           | a,c       | abc,abc,abc | depart,roundabout-exit-1,arrive |
           | a,l       | abc,jkl,jkl | depart,roundabout-exit-2,arrive |
           | a,i       | abc,ghi,ghi | depart,roundabout-exit-3,arrive |
           | a,f       | abc,def,def | depart,roundabout-exit-4,arrive |
           | d,f       | def,def,def | depart,roundabout-exit-1,arrive |
           | d,c       | def,abc,abc | depart,roundabout-exit-2,arrive |
           | d,l       | def,jkl,jkl | depart,roundabout-exit-3,arrive |
           | d,i       | def,ghi,ghi | depart,roundabout-exit-4,arrive |
           | g,i       | ghi,ghi,ghi | depart,roundabout-exit-1,arrive |
           | g,f       | ghi,def,def | depart,roundabout-exit-2,arrive |
           | g,c       | ghi,abc,abc | depart,roundabout-exit-3,arrive |
           | g,l       | ghi,jkl,jkl | depart,roundabout-exit-4,arrive |
           | j,l       | jkl,jkl,jkl | depart,roundabout-exit-1,arrive |
           | j,i       | jkl,ghi,ghi | depart,roundabout-exit-2,arrive |
           | j,f       | jkl,def,def | depart,roundabout-exit-3,arrive |
           | j,c       | jkl,abc,abc | depart,roundabout-exit-4,arrive |

    Scenario: Segregated roads - Not an intersection
        Given the node map
           """
             a   c
           l   b   d
             k   e
           j   h   f
             i   g
           """

        And the ways
           | nodes | junction   | oneway |
           | abc   |            | yes    |
           | def   |            | yes    |
           | ghi   |            | yes    |
           | jkl   |            | yes    |
           | bkheb | roundabout | yes    |

        When I route I should get
           | waypoints | route       | turns                           |
           | a,c       | abc,abc,abc | depart,roundabout-exit-4,arrive |
           | a,l       | abc,jkl,jkl | depart,roundabout-exit-1,arrive |
           | a,i       | abc,ghi,ghi | depart,roundabout-exit-2,arrive |
           | a,f       | abc,def,def | depart,roundabout-exit-3,arrive |
           | d,f       | def,def,def | depart,roundabout-exit-4,arrive |
           | d,c       | def,abc,abc | depart,roundabout-exit-1,arrive |
           | d,l       | def,jkl,jkl | depart,roundabout-exit-2,arrive |
           | d,i       | def,ghi,ghi | depart,roundabout-exit-3,arrive |
           | g,i       | ghi,ghi,ghi | depart,roundabout-exit-4,arrive |
           | g,f       | ghi,def,def | depart,roundabout-exit-1,arrive |
           | g,c       | ghi,abc,abc | depart,roundabout-exit-2,arrive |
           | g,l       | ghi,jkl,jkl | depart,roundabout-exit-3,arrive |
           | j,l       | jkl,jkl,jkl | depart,roundabout-exit-4,arrive |
           | j,i       | jkl,ghi,ghi | depart,roundabout-exit-1,arrive |
           | j,f       | jkl,def,def | depart,roundabout-exit-2,arrive |
           | j,c       | jkl,abc,abc | depart,roundabout-exit-3,arrive |

       Scenario: Collinear in X
        Given the node map
            """
            a b c d f
                e
            """

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                                         |
            | a,e       | ab,ce,ce | depart,roundabout turn right exit-1,arrive    |
            | a,f       | ab,df,df | depart,roundabout turn straight exit-2,arrive |

       Scenario: Collinear in Y
        Given the node map
            """
              a
              b
            e c
              d
              f
            """

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                                         |
            | a,e       | ab,ce,ce | depart,roundabout turn right exit-1,arrive    |
            | a,f       | ab,df,df | depart,roundabout turn straight exit-2,arrive |

       Scenario: Collinear in X,Y
        Given the node map
            """
            a
            b
            c d f
            e
            """

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                                         |
            | a,e       | ab,ce,ce | depart,roundabout turn straight exit-1,arrive |
            | a,f       | ab,df,df | depart,roundabout turn left exit-2,arrive     |

       Scenario: Collinear in X,Y
        Given the node map
            """
            a
            d
            b c f
            e
            """

        And the ways
            | nodes | junction   |
            | ad    |            |
            | bcdb  | roundabout |
            | be    |            |
            | cf    |            |

        When I route I should get
            | waypoints | route    | turns                                         |
            | a,e       | ad,be,be | depart,roundabout turn straight exit-1,arrive |
            | a,f       | ad,cf,cf | depart,roundabout turn left exit-2,arrive     |

       Scenario: Collinear in X,Y
        Given the node map
            """
            a
            c
            d b f
            e
            """

        And the ways
            | nodes | junction   |
            | ac    |            |
            | bcdb  | roundabout |
            | de    |            |
            | bf    |            |

        When I route I should get
            | waypoints | route    | turns                                         |
            | a,e       | ac,de,de | depart,roundabout turn straight exit-1,arrive |
            | a,f       | ac,bf,bf | depart,roundabout turn left exit-2,arrive     |

    Scenario: Enter and Exit -- too complex
        Given the node map
            """
            j   a
              i b
              g   c d
            h   e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | ij     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bigecb | roundabout |

       When I route I should get
           | waypoints | route    | turns                           |
           | a,d       | ab,cd,cd | depart,roundabout-exit-4,arrive |
           | a,f       | ab,ef,ef | depart,roundabout-exit-3,arrive |
           | a,h       | ab,gh,gh | depart,roundabout-exit-2,arrive |
           | d,f       | cd,ef,ef | depart,roundabout-exit-4,arrive |
           | d,h       | cd,gh,gh | depart,roundabout-exit-3,arrive |
           | d,a       | cd,ab,ab | depart,roundabout-exit-1,arrive |
           | f,h       | ef,gh,gh | depart,roundabout-exit-4,arrive |
           | f,a       | ef,ab,ab | depart,roundabout-exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout-exit-1,arrive |
           | h,a       | gh,ab,ab | depart,roundabout-exit-3,arrive |
           | h,d       | gh,cd,cd | depart,roundabout-exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout-exit-1,arrive |

    Scenario: Enter and Exit -- Non-Distinct
        Given the node map
           """
               a
               b
             g   c d
               e
           h   f
           """

       And the ways
            | nodes | junction   |
            | ab    |            |
            | cd    |            |
            | ef    |            |
            | gh    |            |
            | bgecb | roundabout |

       When I route I should get
           | waypoints | route    | turns                           |
           | a,d       | ab,cd,cd | depart,roundabout-exit-3,arrive |
           | a,f       | ab,ef,ef | depart,roundabout-exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout-exit-1,arrive |
           | d,f       | cd,ef,ef | depart,roundabout-exit-3,arrive |
           | d,h       | cd,gh,gh | depart,roundabout-exit-2,arrive |
           | d,a       | cd,ab,ab | depart,roundabout-exit-1,arrive |
           | f,h       | ef,gh,gh | depart,roundabout-exit-3,arrive |
           | f,a       | ef,ab,ab | depart,roundabout-exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout-exit-1,arrive |
           | h,a       | gh,ab,ab | depart,roundabout-exit-3,arrive |
           | h,d       | gh,cd,cd | depart,roundabout-exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout-exit-1,arrive |

    Scenario: Enter and Exit -- Bearing
        Given the node map
           """
               a
               b
           h g   c d
               e
               f
           """

       And the ways
            | nodes | junction   |
            | ab    |            |
            | cd    |            |
            | ef    |            |
            | gh    |            |
            | bgecb | roundabout |

       When I route I should get
           | waypoints | route    | turns                                         | bearing                |
           | a,d       | ab,cd,cd | depart,roundabout turn left exit-3,arrive     | 0->180,180->225,90->0  |
           | a,f       | ab,ef,ef | depart,roundabout turn straight exit-2,arrive | 0->180,180->225,180->0 |
           | a,h       | ab,gh,gh | depart,roundabout turn right exit-1,arrive    | 0->180,180->225,270->0 |

    Scenario: Enter and Exit - Bearings
        Given the node map
            """
                  a

                i b l
            h   g   c   d
                j e k

                  f
            """

       And the ways
            | nodes      | junction   |
            | ab         |            |
            | cd         |            |
            | ef         |            |
            | gh         |            |
            | bigjekclb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                                         | bearing                |
           | a,d       | ab,cd,cd | depart,roundabout turn left exit-3,arrive     | 0->180,180->270,90->0  |
           | a,f       | ab,ef,ef | depart,roundabout turn straight exit-2,arrive | 0->180,180->270,180->0 |
           | a,h       | ab,gh,gh | depart,roundabout turn right exit-1,arrive    | 0->180,180->270,270->0 |

    Scenario: Large radius Roundabout Intersection and ways modelled out: East Mission St, North 7th St
    # http://www.openstreetmap.org/way/348812150
    # Note: grid size is 3 meter, this roundabout is more like 5-10 meters in radius
        Given the node map
           """
                 a

                 b   n

               c       m

           e   d       k   l

               f       j

                 g   i

                 h
           """

       And the ways
            | nodes       | junction   | highway  | name            |
            | ab          |            | tertiary | North 7th St    |
            | ed          |            | tertiary | East Mission St |
            | hg          |            | tertiary | North 7th St    |
            | lk          |            | tertiary | East Mission St |
            | bcdfgijkmnb | roundabout | tertiary | Roundabout      |

       When I route I should get
           | waypoints | route                                           | turns                                         |
           | a,e       | North 7th St,East Mission St,East Mission St    | depart,roundabout turn right exit-1,arrive    |
           | a,h       | North 7th St,North 7th St,North 7th St          | depart,roundabout turn straight exit-2,arrive |
           | a,l       | North 7th St,East Mission St,East Mission St    | depart,roundabout turn left exit-3,arrive     |
           | h,l       | North 7th St,East Mission St,East Mission St    | depart,roundabout turn right exit-1,arrive    |
           | h,a       | North 7th St,North 7th St,North 7th St          | depart,roundabout turn straight exit-2,arrive |
           | h,e       | North 7th St,East Mission St,East Mission St    | depart,roundabout turn left exit-3,arrive     |
           | e,h       | East Mission St,North 7th St,North 7th St       | depart,roundabout turn right exit-1,arrive    |
           | e,l       | East Mission St,East Mission St,East Mission St | depart,roundabout turn straight exit-2,arrive |
           | e,a       | East Mission St,North 7th St,North 7th St       | depart,roundabout turn left exit-3,arrive     |
           | l,a       | East Mission St,North 7th St,North 7th St       | depart,roundabout turn right exit-1,arrive    |
           | l,e       | East Mission St,East Mission St,East Mission St | depart,roundabout turn straight exit-2,arrive |
           | l,h       | East Mission St,North 7th St,North 7th St       | depart,roundabout turn left exit-3,arrive     |

    Scenario: Enter and Exit - Traffic Signals
        Given the node map
            """
                a
              i b l
            h g   c d
              j e k
                f
            """

       And the nodes
            | node | highway         |
            | i    | traffic_signals |
            | j    | traffic_signals |
            | k    | traffic_signals |
            | l    | traffic_signals |

       And the ways
            | nodes     | junction   |
            | ab        |            |
            | cd        |            |
            | ef        |            |
            | gh        |            |
            | bigjekclb | roundabout |

       When I route I should get
           | waypoints | route    | turns                                         |
           | a,d       | ab,cd,cd | depart,roundabout turn left exit-3,arrive     |
           | a,f       | ab,ef,ef | depart,roundabout turn straight exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout turn right exit-1,arrive    |
           | d,f       | cd,ef,ef | depart,roundabout turn left exit-3,arrive     |
           | d,h       | cd,gh,gh | depart,roundabout turn straight exit-2,arrive |
           | d,a       | cd,ab,ab | depart,roundabout turn right exit-1,arrive    |
           | f,h       | ef,gh,gh | depart,roundabout turn left exit-3,arrive     |
           | f,a       | ef,ab,ab | depart,roundabout turn straight exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout turn right exit-1,arrive    |
           | h,a       | gh,ab,ab | depart,roundabout turn left exit-3,arrive     |
           | h,d       | gh,cd,cd | depart,roundabout turn straight exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout turn right exit-1,arrive    |

    #http://www.openstreetmap.org/#map=19/41.03275/-2.18990
    #at some point we probably want to recognise these situations and don't mention the roundabout at all here
    Scenario: Enter And Exit Throughabout
        Given the node map
            """
                      h

            c b   d       e   f

              a       g
            """

        And the ways
            | nodes | highway       | name    | junction   | oneway |
            | dghd  | tertiary_link |         | roundabout |        |
            | cbdef | trunk         | through |            | no     |
            | ab    | residential   | in      |            |        |

        When I route I should get
            | waypoints | turns                                                    | route                      |
            | a,f       | depart,turn right,roundabout turn straight exit-1,arrive | in,through,through,through |
