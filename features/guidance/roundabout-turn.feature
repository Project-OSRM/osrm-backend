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
           | waypoints | route    | turns | locations |
            | a,d | ab,cd,cd | depart,roundabout turn left exit-3,arrive | a,?,d |
            | a,f | ab,ef,ef | depart,roundabout turn straight exit-2,arrive | a,?,f |
            | a,h | ab,gh,gh | depart,roundabout turn right exit-1,arrive | a,?,h |
            | d,f | cd,ef,ef | depart,roundabout turn left exit-3,arrive | d,?,f |
            | d,h | cd,gh,gh | depart,roundabout turn straight exit-2,arrive | d,?,h |
            | d,a | cd,ab,ab | depart,roundabout turn right exit-1,arrive | d,?,a |
            | f,h | ef,gh,gh | depart,roundabout turn left exit-3,arrive | f,?,h |
            | f,a | ef,ab,ab | depart,roundabout turn straight exit-2,arrive | f,?,a |
            | f,d | ef,cd,cd | depart,roundabout turn right exit-1,arrive | f,?,d |
            | h,a | gh,ab,ab | depart,roundabout turn left exit-3,arrive | h,?,a |
            | h,d | gh,cd,cd | depart,roundabout turn straight exit-2,arrive | h,?,d |
            | h,f | gh,ef,ef | depart,roundabout turn right exit-1,arrive | h,?,f |

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
           | waypoints | route    | turns | locations |
            | a,d | ab,cd,cd | depart,roundabout turn left exit-3,arrive | a,?,d |
            | a,f | ab,ef,ef | depart,roundabout turn straight exit-2,arrive | a,?,f |
            | a,h | ab,gh,gh | depart,roundabout turn right exit-1,arrive | a,?,h |
            | d,f | cd,ef,ef | depart,roundabout turn left exit-3,arrive | d,?,f |
            | d,h | cd,gh,gh | depart,roundabout turn straight exit-2,arrive | d,?,h |
            | d,a | cd,ab,ab | depart,roundabout turn right exit-1,arrive | d,?,a |
            | f,h | ef,gh,gh | depart,roundabout turn left exit-3,arrive | f,?,h |
            | f,a | ef,ab,ab | depart,roundabout turn straight exit-2,arrive | f,?,a |
            | f,d | ef,cd,cd | depart,roundabout turn right exit-1,arrive | f,?,d |
            | h,a | gh,ab,ab | depart,roundabout turn left exit-3,arrive | h,?,a |
            | h,d | gh,cd,cd | depart,roundabout turn straight exit-2,arrive | h,?,d |
            | h,f | gh,ef,ef | depart,roundabout turn right exit-1,arrive | h,?,f |

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
           | waypoints | route          | turns | locations |
            | a,c | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | a,b,c |
            | a,e | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | a,b,e |
            | a,g | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | a,b,g |
            | d,e | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | d,c,e |
            | d,g | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | d,c,g |
            | d,b | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | d,c,b |
            | f,g | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | f,e,g |
            | f,b | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | f,e,b |
            | f,c | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | f,e,c |
            | h,b | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | h,g,b |
            | h,c | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | h,g,c |
            | h,e | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive | h,g,e |

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
           | waypoints | route       | turns | locations |
            | b,d | bcegb,cd,cd | depart,exit roundabout right,arrive | b,c,d |
            | b,f | bcegb,ef,ef | depart,exit roundabout right,arrive | b,e,f |
            | b,h | bcegb,gh,gh | depart,exit roundabout right,arrive | b,g,h |
            | c,f | bcegb,ef,ef | depart,exit roundabout right,arrive | c,e,f |
            | c,h | bcegb,gh,gh | depart,exit roundabout right,arrive | c,g,h |
            | c,a | bcegb,ab,ab | depart,exit roundabout right,arrive | c,b,a |
            | e,h | bcegb,gh,gh | depart,exit roundabout right,arrive | e,g,h |
            | e,a | bcegb,ab,ab | depart,exit roundabout right,arrive | e,b,a |
            | e,d | bcegb,cd,cd | depart,exit roundabout right,arrive | e,c,d |
            | g,a | bcegb,ab,ab | depart,exit roundabout right,arrive | g,b,a |
            | g,d | bcegb,cd,cd | depart,exit roundabout right,arrive | g,c,d |
            | g,f | bcegb,ef,ef | depart,exit roundabout right,arrive | g,e,f |
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
           | waypoints | route       | turns | locations |
            | b,c | bcegb,bcegb | depart,arrive | b,c |
            | b,e | bcegb,bcegb | depart,arrive | b,e |
            | b,g | bcegb,bcegb | depart,arrive | b,g |
            | c,e | bcegb,bcegb | depart,arrive | c,e |
            | c,g | bcegb,bcegb | depart,arrive | c,g |
            | c,b | bcegb,bcegb | depart,arrive | c,b |
            | e,g | bcegb,bcegb | depart,arrive | e,g |
            | e,b | bcegb,bcegb | depart,arrive | e,b |
            | e,c | bcegb,bcegb | depart,arrive | e,c |
            | g,b | bcegb,bcegb | depart,arrive | g,b |
            | g,c | bcegb,bcegb | depart,arrive | g,c |
            | g,e | bcegb,bcegb | depart,arrive | g,e |

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
           | waypoints | route              | turns | locations |
            | a,c | abc,abc,abc | depart,exit roundabout right,arrive | a,a,c |
            | a,l | abc,jkl,jkl,jkl | depart,roundabout-exit-2,exit roundabout straight,arrive | a,?,j,l |
            | a,i | abc,ghi,ghi,ghi | depart,roundabout-exit-3,exit roundabout straight,arrive | a,?,g,i |
            | a,f | abc,def,def,def | depart,roundabout-exit-4,exit roundabout straight,arrive | a,?,d,f |
            | d,f | def,def,def | depart,exit roundabout right,arrive | d,d,f |
            | d,c | def,abc,abc,abc | depart,roundabout-exit-2,exit roundabout straight,arrive | d,?,a,c |
            | d,l | def,jkl,jkl,jkl | depart,roundabout-exit-3,exit roundabout straight,arrive | d,?,j,l |
            | d,i | def,ghi,ghi,ghi | depart,roundabout-exit-4,exit roundabout straight,arrive | d,?,g,i |
            | g,i | ghi,ghi,ghi | depart,exit roundabout right,arrive | g,g,i |
            | g,f | ghi,def,def,def | depart,roundabout-exit-2,exit roundabout straight,arrive | g,?,d,f |
            | g,c | ghi,abc,abc,abc | depart,roundabout-exit-3,exit roundabout straight,arrive | g,?,a,c |
            | g,l | ghi,jkl,jkl,jkl | depart,roundabout-exit-4,exit roundabout straight,arrive | g,?,j,l |
            | j,l | jkl,jkl,jkl | depart,exit roundabout right,arrive | j,j,l |
            | j,i | jkl,ghi,ghi,ghi | depart,roundabout-exit-2,exit roundabout straight,arrive | j,?,g,i |
            | j,f | jkl,def,def,def | depart,roundabout-exit-3,exit roundabout straight,arrive | j,?,d,f |
            | j,c | jkl,abc,abc,abc | depart,roundabout-exit-4,exit roundabout straight,arrive | j,?,a,c |

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
           | waypoints | route            | turns | locations |
            | a,c | abc,abc,abc,abc | depart,roundabout-exit-4,exit roundabout right,arrive | a,a,a,c |
            | a,l | abc,jkl,jkl,jkl | depart,roundabout-exit-1,exit roundabout right,arrive | a,?,j,l |
            | a,i | abc,ghi,ghi,ghi | depart,roundabout-exit-2,exit roundabout right,arrive | a,?,g,i |
            | a,f | abc,def,def,def | depart,roundabout-exit-3,exit roundabout right,arrive | a,?,d,f |
            | d,f | def,def,def,def | depart,roundabout-exit-4,exit roundabout right,arrive | d,d,d,f |
            | d,c | def,abc,abc,abc | depart,roundabout-exit-1,exit roundabout right,arrive | d,?,a,c |
            | d,l | def,jkl,jkl,jkl | depart,roundabout-exit-2,exit roundabout right,arrive | d,?,j,l |
            | d,i | def,ghi,ghi,ghi | depart,roundabout-exit-3,exit roundabout right,arrive | d,?,g,i |
            | g,i | ghi,ghi,ghi,ghi | depart,roundabout-exit-4,exit roundabout right,arrive | g,g,g,i |
            | g,f | ghi,def,def,def | depart,roundabout-exit-1,exit roundabout right,arrive | g,?,d,f |
            | g,c | ghi,abc,abc,abc | depart,roundabout-exit-2,exit roundabout right,arrive | g,?,a,c |
            | g,l | ghi,jkl,jkl,jkl | depart,roundabout-exit-3,exit roundabout right,arrive | g,?,j,l |
            | j,l | jkl,jkl,jkl,jkl | depart,roundabout-exit-4,exit roundabout right,arrive | j,j,j,l |
            | j,i | jkl,ghi,ghi,ghi | depart,roundabout-exit-1,exit roundabout right,arrive | j,?,g,i |
            | j,f | jkl,def,def,def | depart,roundabout-exit-2,exit roundabout right,arrive | j,?,d,f |
            | j,c | jkl,abc,abc,abc | depart,roundabout-exit-3,exit roundabout right,arrive | j,?,a,c |

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
            | waypoints | route    | turns | locations |
            | a,e | ab,ce,ce | depart,roundabout turn right exit-1,arrive | a,?,e |
            | a,f | ab,df,df | depart,roundabout turn straight exit-2,arrive | a,?,f |

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
            | waypoints | route    | turns | locations |
            | a,e | ab,ce,ce | depart,roundabout turn right exit-1,arrive | a,?,e |
            | a,f | ab,df,df | depart,roundabout turn straight exit-2,arrive | a,?,f |

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
            | waypoints | route    | turns | locations |
            | a,e | ab,ce,ce | depart,roundabout turn straight exit-1,arrive | a,?,e |
            | a,f | ab,df,df | depart,roundabout turn left exit-2,arrive | a,?,f |

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
            | waypoints | route    | turns | locations |
            | a,e | ad,be,be | depart,roundabout turn straight exit-1,arrive | a,?,e |
            | a,f | ad,cf,cf | depart,roundabout turn left exit-2,arrive | a,?,f |

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
            | waypoints | route    | turns | locations |
            | a,e | ac,de,de | depart,roundabout turn straight exit-1,arrive | a,?,e |
            | a,f | ac,bf,bf | depart,roundabout turn left exit-2,arrive | a,?,f |

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
           | waypoints | route       | turns | locations |
            | a,d | ab,cd,cd,cd | depart,roundabout-exit-4,exit roundabout right,arrive | a,?,c,d |
            | a,f | ab,ef,ef,ef | depart,roundabout-exit-3,exit roundabout right,arrive | a,?,e,f |
            | a,h | ab,gh,gh,gh | depart,roundabout-exit-2,exit roundabout right,arrive | a,?,g,h |
            | d,f | cd,ef,ef,ef | depart,roundabout-exit-4,exit roundabout right,arrive | d,?,e,f |
            | d,h | cd,gh,gh,gh | depart,roundabout-exit-3,exit roundabout right,arrive | d,?,g,h |
            | d,a | cd,ab,ab,ab | depart,roundabout-exit-1,exit roundabout right,arrive | d,?,a,a |
            | f,h | ef,gh,gh,gh | depart,roundabout-exit-4,exit roundabout right,arrive | f,?,g,h |
            | f,a | ef,ab,ab,ab | depart,roundabout-exit-2,exit roundabout right,arrive | f,?,a,a |
            | f,d | ef,cd,cd,cd | depart,roundabout-exit-1,exit roundabout right,arrive | f,?,c,d |
            | h,a | gh,ab,ab,ab | depart,roundabout-exit-3,exit roundabout right,arrive | h,?,a,a |
            | h,d | gh,cd,cd,cd | depart,roundabout-exit-2,exit roundabout right,arrive | h,?,c,d |
            | h,f | gh,ef,ef,ef | depart,roundabout-exit-1,exit roundabout right,arrive | h,?,e,f |

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
           | waypoints | route       | turns | locations |
            | a,d | ab,cd,cd,cd | depart,roundabout-exit-3,exit roundabout right,arrive | a,?,c,d |
            | a,f | ab,ef,ef,ef | depart,roundabout-exit-2,exit roundabout right,arrive | a,?,e,f |
            | a,h | ab,gh,gh,gh | depart,roundabout-exit-1,exit roundabout straight,arrive | a,?,g,h |
            | d,f | cd,ef,ef,ef | depart,roundabout-exit-3,exit roundabout right,arrive | d,?,e,f |
            | d,h | cd,gh,gh,gh | depart,roundabout-exit-2,exit roundabout straight,arrive | d,?,g,h |
            | d,a | cd,ab,ab,ab | depart,roundabout-exit-1,exit roundabout right,arrive | d,?,a,a |
            | f,h | ef,gh,gh,gh | depart,roundabout-exit-3,exit roundabout straight,arrive | f,?,g,h |
            | f,a | ef,ab,ab,ab | depart,roundabout-exit-2,exit roundabout right,arrive | f,?,a,a |
            | f,d | ef,cd,cd,cd | depart,roundabout-exit-1,exit roundabout right,arrive | f,?,c,d |
            | h,a | gh,ab,ab,ab | depart,roundabout-exit-3,exit roundabout right,arrive | h,?,a,a |
            | h,d | gh,cd,cd,cd | depart,roundabout-exit-2,exit roundabout right,arrive | h,?,c,d |
            | h,f | gh,ef,ef,ef | depart,roundabout-exit-1,exit roundabout right,arrive | h,?,e,f |

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
           | waypoints | route    | turns                                         | bearing | locations |
            | a,d | ab,cd,cd | depart,roundabout turn left exit-3,arrive | 0->180,180->225,90->0 | a,?,d |
            | a,f | ab,ef,ef | depart,roundabout turn straight exit-2,arrive | 0->180,180->225,180->0 | a,?,f |
            | a,h | ab,gh,gh | depart,roundabout turn right exit-1,arrive | 0->180,180->225,270->0 | a,?,h |

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
           | waypoints | route    | turns                                         | bearing | locations |
            | a,d | ab,cd,cd | depart,roundabout turn left exit-3,arrive | 0->180,180->270,90->0 | a,?,d |
            | a,f | ab,ef,ef | depart,roundabout turn straight exit-2,arrive | 0->180,180->270,180->0 | a,?,f |
            | a,h | ab,gh,gh | depart,roundabout turn right exit-1,arrive | 0->180,180->270,270->0 | a,?,h |

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
           | waypoints | route                                           | turns | locations |
            | a,e | North 7th St,East Mission St,East Mission St | depart,roundabout turn right exit-1,arrive | a,t,e |
            | a,h | North 7th St,North 7th St,North 7th St | depart,roundabout turn straight exit-2,arrive | a,N,h |
            | a,l | North 7th St,East Mission St,East Mission St | depart,roundabout turn left exit-3,arrive | a,t,l |
            | h,l | North 7th St,East Mission St,East Mission St | depart,roundabout turn right exit-1,arrive | h,t,l |
            | h,a | North 7th St,North 7th St,North 7th St | depart,roundabout turn straight exit-2,arrive | h,N,a |
            | h,e | North 7th St,East Mission St,East Mission St | depart,roundabout turn left exit-3,arrive | h,t,e |
            | e,h | East Mission St,North 7th St,North 7th St | depart,roundabout turn right exit-1,arrive | e,t,h |
            | e,l | East Mission St,East Mission St,East Mission St | depart,roundabout turn straight exit-2,arrive | e,E,l |
            | e,a | East Mission St,North 7th St,North 7th St | depart,roundabout turn left exit-3,arrive | e,t,a |
            | l,a | East Mission St,North 7th St,North 7th St | depart,roundabout turn right exit-1,arrive | l,t,a |
            | l,e | East Mission St,East Mission St,East Mission St | depart,roundabout turn straight exit-2,arrive | l,E,e |
            | l,h | East Mission St,North 7th St,North 7th St | depart,roundabout turn left exit-3,arrive | l,t,h |

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
           | waypoints | route    | turns | locations |
            | a,d | ab,cd,cd | depart,roundabout turn left exit-3,arrive | a,?,d |
            | a,f | ab,ef,ef | depart,roundabout turn straight exit-2,arrive | a,?,f |
            | a,h | ab,gh,gh | depart,roundabout turn right exit-1,arrive | a,?,h |
            | d,f | cd,ef,ef | depart,roundabout turn left exit-3,arrive | d,?,f |
            | d,h | cd,gh,gh | depart,roundabout turn straight exit-2,arrive | d,?,h |
            | d,a | cd,ab,ab | depart,roundabout turn right exit-1,arrive | d,?,a |
            | f,h | ef,gh,gh | depart,roundabout turn left exit-3,arrive | f,?,h |
            | f,a | ef,ab,ab | depart,roundabout turn straight exit-2,arrive | f,?,a |
            | f,d | ef,cd,cd | depart,roundabout turn right exit-1,arrive | f,?,d |
            | h,a | gh,ab,ab | depart,roundabout turn left exit-3,arrive | h,?,a |
            | h,d | gh,cd,cd | depart,roundabout turn straight exit-2,arrive | h,?,d |
            | h,f | gh,ef,ef | depart,roundabout turn right exit-1,arrive | h,?,f |

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
            | waypoints | turns                    | route | locations |
            | a,f | depart,turn right,arrive | in,through,through | a,?,f |
