@routing  @guidance
Feature: Simple Turns

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Four Way Intersection
        Given the node map
            """
              c
            a b e
              d
            """

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | db     | primary |
            | eb     | primary |

       When I route I should get
            | waypoints | route    | turns                    | locations |
            | a,c       | ab,cb,cb | depart,turn left,arrive  | a,b,c     |
            | a,e       | ab,eb    | depart,arrive            | a,e       |
            | a,d       | ab,db,db | depart,turn right,arrive | a,b,d     |
            | c,a       | cb,ab,ab | depart,turn right,arrive | c,b,a     |
            | c,d       | cb,db    | depart,arrive            | c,d       |
            | c,e       | cb,eb,eb | depart,turn left,arrive  | c,b,e     |
            | d,a       | db,ab,ab | depart,turn left,arrive  | d,b,a     |
            | d,c       | db,cb    | depart,arrive            | d,c       |
            | d,e       | db,eb,eb | depart,turn right,arrive | d,b,e     |
            | e,a       | eb,ab    | depart,arrive            | e,a       |
            | e,c       | eb,cb,cb | depart,turn right,arrive | e,b,c     |
            | e,d       | eb,db,db | depart,turn left,arrive  | e,b,d     |

    Scenario: Rotated Four Way Intersection
        Given the node map
            """
            a   c
              b
            d   e
            """

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | db     | primary |
            | eb     | primary |

       When I route I should get
            | waypoints | route    | turns                    | locations |
            | a,c       | ab,cb,cb | depart,turn left,arrive  | a,b,c     |
            | a,e       | ab,eb    | depart,arrive            | a,e       |
            | a,d       | ab,db,db | depart,turn right,arrive | a,b,d     |
            | c,a       | cb,ab,ab | depart,turn right,arrive | c,b,a     |
            | c,d       | cb,db    | depart,arrive            | c,d       |
            | c,e       | cb,eb,eb | depart,turn left,arrive  | c,b,e     |
            | d,a       | db,ab,ab | depart,turn left,arrive  | d,b,a     |
            | d,c       | db,cb    | depart,arrive            | d,c       |
            | d,e       | db,eb,eb | depart,turn right,arrive | d,b,e     |
            | e,a       | eb,ab    | depart,arrive            | e,a       |
            | e,c       | eb,cb,cb | depart,turn right,arrive | e,b,c     |
            | e,d       | eb,db,db | depart,turn left,arrive  | e,b,d     |


    Scenario: Four Way Intersection Through Street
        Given the node map
            """
              c
            a b e
              d
            """

        And the ways
            | nodes  | highway |
            | abe    | primary |
            | cb     | primary |
            | db     | primary |

       When I route I should get
            | waypoints | route      | turns                    | locations |
            | a,c       | abe,cb,cb  | depart,turn left,arrive  | a,b,c     |
            | a,e       | abe,abe    | depart,arrive            | a,e       |
            | a,d       | abe,db,db  | depart,turn right,arrive | a,b,d     |
            | c,a       | cb,abe,abe | depart,turn right,arrive | c,b,a     |
            | c,d       | cb,db      | depart,arrive            | c,d       |
            | c,e       | cb,abe,abe | depart,turn left,arrive  | c,b,e     |
            | d,a       | db,abe,abe | depart,turn left,arrive  | d,b,a     |
            | d,c       | db,cb      | depart,arrive            | d,c       |
            | d,e       | db,abe,abe | depart,turn right,arrive | d,b,e     |
            | e,a       | abe,abe    | depart,arrive            | e,a       |
            | e,c       | abe,cb,cb  | depart,turn right,arrive | e,b,c     |
            | e,d       | abe,db,db  | depart,turn left,arrive  | e,b,d     |

    Scenario: Four Way Intersection Double Through Street
        Given the node map
            """
              c
            a b e
              d
            """

        And the ways
            | nodes  | highway |
            | abe    | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route       | turns                    | locations |
            | a,c       | abe,cbd,cbd | depart,turn left,arrive  | a,b,c     |
            | a,e       | abe,abe     | depart,arrive            | a,e       |
            | a,d       | abe,cbd,cbd | depart,turn right,arrive | a,b,d     |
            | c,a       | cbd,abe,abe | depart,turn right,arrive | c,b,a     |
            | c,d       | cbd,cbd     | depart,arrive            | c,d       |
            | c,e       | cbd,abe,abe | depart,turn left,arrive  | c,b,e     |
            | d,a       | cbd,abe,abe | depart,turn left,arrive  | d,b,a     |
            | d,c       | cbd,cbd     | depart,arrive            | d,c       |
            | d,e       | cbd,abe,abe | depart,turn right,arrive | d,b,e     |
            | e,a       | abe,abe     | depart,arrive            | e,a       |
            | e,c       | abe,cbd,cbd | depart,turn right,arrive | e,b,c     |
            | e,d       | abe,cbd,cbd | depart,turn left,arrive  | e,b,d     |

    Scenario: Three Way Intersection
        Given the node map
            """
              c
            a b d
            """

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | db     | primary |

       When I route I should get
            | waypoints | route    | turns                    | locations |
            | a,c       | ab,cb,cb | depart,turn left,arrive  | a,b,c     |
            | a,d       | ab,db    | depart,arrive            | a,d       |
            | d,c       | db,cb,cb | depart,turn right,arrive | d,b,c     |
            | d,a       | db,ab    | depart,arrive            | d,a       |

    Scenario: Three Way Intersection - Meeting Oneways
        Given the node map
            """
              c
            a b d
            """

        And the ways
            | nodes  | highway | oneway |
            | ab     | primary | yes    |
            | bc     | primary | yes    |
            | db     | primary | yes    |

       When I route I should get
            | waypoints | route    | turns                    | locations |
            | a,c       | ab,bc,bc | depart,turn left,arrive  | a,b,c     |
            | d,c       | db,bc,bc | depart,turn right,arrive | d,b,c     |

    Scenario: Three Way Intersection on Through Street
        Given the node map
            """
              d
            a b c
            """

        And the ways
            | nodes  | highway |
            | abc    | primary |
            | db     | primary |

       When I route I should get
            | waypoints | route     | turns                    | locations |
            | a,c       | abc,abc   | depart,arrive            | a,c       |
            | a,d       | abc,db,db | depart,turn left,arrive  | a,b,d     |
            | c,a       | abc,abc   | depart,arrive            | c,a       |
            | c,d       | abc,db,db | depart,turn right,arrive | c,b,d     |

     Scenario: High Degree Intersection
        Given the node map
            """
            i   b   c


            h   a   d


            g   f   e
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | ac    | primary |
            | ad    | primary |
            | ae    | primary |
            | af    | primary |
            | ag    | primary |
            | ah    | primary |
            | ai    | primary |

        When I route I should get
            | waypoints | route    | turns                           | locations |
            | b,c       | ab,ac,ac | depart,turn sharp left,arrive   | b,a,c     |
            | b,d       | ab,ad,ad | depart,turn left,arrive         | b,a,d     |
            | b,e       | ab,ae,ae | depart,turn slight left,arrive  | b,a,e     |
            | b,f       | ab,af    | depart,arrive                   | b,f       |
            | b,g       | ab,ag,ag | depart,turn slight right,arrive | b,a,g     |
            | b,h       | ab,ah,ah | depart,turn right,arrive        | b,a,h     |
            | b,i       | ab,ai,ai | depart,turn sharp right,arrive  | b,a,i     |

    Scenario: Disturbed High Degree Intersection
        Given the node map
            """
                b
            i       c

            h   a   d

            g       e
                f
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | ac    | primary |
            | ad    | primary |
            | ae    | primary |
            | af    | primary |
            | ag    | primary |
            | ah    | primary |
            | ai    | primary |

        When I route I should get
            | waypoints | route    | turns                           | locations |
            | b,c       | ab,ac,ac | depart,turn sharp left,arrive   | b,a,c     |
            | b,d       | ab,ad,ad | depart,turn left,arrive         | b,a,d     |
            | b,e       | ab,ae,ae | depart,turn slight left,arrive  | b,a,e     |
            | b,f       | ab,af    | depart,arrive                   | b,f       |
            | b,g       | ab,ag,ag | depart,turn slight right,arrive | b,a,g     |
            | b,h       | ab,ah,ah | depart,turn right,arrive        | b,a,h     |
            | b,i       | ab,ai,ai | depart,turn sharp right,arrive  | b,a,i     |

    Scenario: Turn instructions at high latitude
        Given the node locations
            | node | lat       | lon      |
            | a    | 55.68740  | 12.52430 |
            | b    | 55.68745  | 12.52409 |
            | c    | 55.68711  | 12.52383 |
            | d    | 55.68745  | 12.52450 |
            | e    | 55.68755  | 12.52450 |
            | x    | -55.68740 | 12.52430 |
            | y    | -55.68745 | 12.52409 |
            | z    | -55.68711 | 12.52383 |
            | v    | -55.68745 | 12.52450 |
            | w    | -55.68755 | 12.52450 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bd    |
            | be    |
            | xy    |
            | yz    |
            | vy    |
            | wy    |

        When I route I should get
            | from | to | route    | turns                    | locations |
            | a    | c  | ab,bc,bc | depart,turn left,arrive  | a,b,c     |
            | c    | a  | bc,ab,ab | depart,turn right,arrive | c,b,a     |
            | x    | z  | xy,yz,yz | depart,turn right,arrive | x,y,z     |
            | z    | x  | yz,xy,xy | depart,turn left,arrive  | z,y,x     |

    Scenario: Three Way Similar Sharp Turns
        Given the node map
            """
            a       b
            c
              d
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | bd    | primary |

        When I route I should get
            | waypoints | route    | turns                          | locations |
            | a,c       | ab,bc,bc | depart,turn sharp right,arrive | a,b,c     |
            | a,d       | ab,bd,bd | depart,turn sharp right,arrive | a,b,d     |
            | d,c       | bd,bc,bc | depart,turn sharp left,arrive  | d,b,c     |
            | d,a       | bd,ab,ab | depart,turn sharp left,arrive  | d,b,a     |

    Scenario: Left Turn Assignment (1)
        Given the node map
            """
                    d
            a   b   c
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                          | locations |
            | a,d       | abc,bd,bd | depart,turn slight left,arrive | a,b,d     |

    Scenario: Left Turn Assignment (2)
        Given the node map
            """
                    d

            a   b   c
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   | locations |
            | a,d       | abc,bd,bd | depart,turn left,arrive | a,b,d     |

    Scenario: Left Turn Assignment (3)
        Given the node map
            """
                  d


            a   b   c
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   | locations |
            | a,d       | abc,bd,bd | depart,turn left,arrive | a,b,d     |

    Scenario: Left Turn Assignment (4)
        Given the node map
            """
                d



            a   b   c
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   | locations |
            | a,d       | abc,bd,bd | depart,turn left,arrive | a,b,d     |

    Scenario: Left Turn Assignment (5)
        Given the node map
            """
              d


            a   b   c
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   | locations |
            | a,d       | abc,bd,bd | depart,turn left,arrive | a,b,d     |

    Scenario: Left Turn Assignment (6)
        Given the node map
            """
            d

            a   b   c
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                         | locations |
            | a,d       | abc,bd,bd | depart,turn sharp left,arrive | a,b,d     |

    Scenario: Left Turn Assignment (7)
        Given the node map
            """
            d
            a   b   c
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                         | locations |
            | a,d       | abc,bd,bd | depart,turn sharp left,arrive | a,b,d     |

    Scenario: Right Turn Assignment (1)
        Given the node map
            """
                e
            a   b   c
                    d
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn slight right,arrive | a,b,d     |

    Scenario: Right Turn Assignment (2)
        Given the node map
            """
                e
            a   b   c

                    d
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive | a,b,d     |

    Scenario: Right Turn Assignment (3)
        Given the node map
            """
                e
            a   b   c


                  d
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive | a,b,d     |

    Scenario: Right Turn Assignment (4)
        Given the node map
            """
                e
            a   b   c



                d
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive | a,b,d     |

    Scenario: Right Turn Assignment (5)
        Given the node map
            """
                e
            a   b   c


              d
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive | a,b,d     |

    Scenario: Right Turn Assignment (6)
        Given the node map
            """
                e
            a   b   c

            d
            """


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                          | locations |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive | a,b,d     |

    Scenario: Right Turn Assignment (7)
        Given the node map
            """
                e
            a   b   c
            d
            """


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                          | locations |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive | a,b,d     |

   Scenario: Right Turn Assignment Two Turns
        Given the node map
            """
                f
            a   b   c

            d e
            """


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                          | locations |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive | a,b,d     |
            | a,e       | abc,be,be | depart,turn right,arrive       | a,b,e     |

   Scenario: Right Turn Assignment Two Turns (2)
        Given the node map
            """
                f   c
            a   b
                    e
                  d
            """


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive        | a,b,d     |
            | a,e       | abc,be,be | depart,turn slight right,arrive | a,b,e     |

   Scenario: Right Turn Assignment Two Turns (3)
        Given the node map
            """
                f
            a   b   c
                    e
                  d
            """


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive        | a,b,d     |
            | a,e       | abc,be,be | depart,turn slight right,arrive | a,b,e     |

   Scenario: Right Turn Assignment Two Turns (4)
        Given the node map
            """
                f
            a   b   c

                d   e
            """


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive        | a,b,d     |
            | a,e       | abc,be,be | depart,turn slight right,arrive | a,b,e     |

   Scenario: Right Turn Assignment Three Turns
        Given the node map
            """
                g
            a   b   c
              d   f
                e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |
            | bg    | primary |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive  | a,b,d     |
            | a,e       | abc,be,be | depart,turn right,arrive        | a,b,e     |
            | a,f       | abc,bf,bf | depart,turn slight right,arrive | a,b,f     |

    Scenario: Slight Turn involving Oneways
        Given the node map
            """
                a

                b   e
            d
                c
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | yes    |
            | dbe   | primary | no     |

        When I route I should get
            | waypoints | route   | turns         | locations |
            | a,c       | abc,abc | depart,arrive | a,c       |
            | d,e       | dbe,dbe | depart,arrive | d,e       |
            | e,d       | dbe,dbe | depart,arrive | e,d       |

    Scenario: Slight Turn involving Oneways
        Given the node map
            """
                  a


                b   e
            d
                c
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | yes    |
            | dbe   | primary | no     |

        When I route I should get
            | waypoints | route   | turns         | locations |
            | a,c       | abc,abc | depart,arrive | a,c       |
            | d,e       | dbe,dbe | depart,arrive | d,e       |
            | e,d       | dbe,dbe | depart,arrive | e,d       |


    Scenario: Slight Turn involving Oneways - Name Change
        Given the node map
            """
                a

                b   e
            d
                c
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | yes    |
            | db    | primary | no     |
            | be    | primary | no     |

        When I route I should get
            | waypoints | route   | turns         | locations |
            | a,c       | abc,abc | depart,arrive | a,c       |
            | d,e       | db,be   | depart,arrive | d,e       |
            | e,d       | be,db   | depart,arrive | e,d       |

     Scenario: Right Turn Assignment Three Conflicting Turns with invalid - 1
        Given the node map
            """
                g
            a   b   c

              d e f
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | no     |
            | db    | primary | yes    |
            | eb    | primary | no     |
            | fb    | primary | no     |
            | bg    | primary | no     |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,e       | abc,eb,eb | depart,turn right,arrive        | a,b,e     |
            | a,f       | abc,fb,fb | depart,turn slight right,arrive | a,b,f     |

     Scenario: Right Turn Assignment Three Conflicting Turns with invalid - 2
        Given the node map
            """
                g
            a   b   c

              d e f
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | yes    |
            | bd    | primary | yes    |
            | eb    | primary | yes    |
            | bf    | primary | yes    |
            | bg    | primary | yes    |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn right,arrive        | a,b,d     |
            | a,f       | abc,bf,bf | depart,turn slight right,arrive | a,b,f     |

    Scenario: Right Turn Assignment Three Conflicting Turns with invalid - 3
        Given the node map
            """
                g
            a   b   c

              d e f
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | no     |
            | db    | primary | no     |
            | be    | primary | no     |
            | fb    | primary | yes    |
            | bg    | primary | no     |

        When I route I should get
            | waypoints | route     | turns                          | locations |
            | a,d       | abc,db,db | depart,turn sharp right,arrive | a,b,d     |
            | a,e       | abc,be,be | depart,turn right,arrive       | a,b,e     |

    Scenario: Conflicting Turns with well distinguished turn
        Given the node map
            """
            a     b     c

            f           d
                        e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn slight right,arrive | a,b,d     |
            | a,e       | abc,be,be | depart,turn right,arrive        | a,b,e     |
            | a,f       | abc,bf,bf | depart,turn sharp right,arrive  | a,b,f     |

    Scenario: Conflicting Turns with well distinguished turn (back)
        Given the node map
            """
            a     b     c

            d           f
              e
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           | locations |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive  | a,b,d     |
            | a,e       | abc,be,be | depart,turn right,arrive        | a,b,e     |
            | a,f       | abc,bf,bf | depart,turn slight right,arrive | a,b,f     |

    Scenario: Turn Lane on Splitting up Road
        Given the node map
            """
            g - - - f -
                         ' .
                    . h - - e - - c - - d
            a - - b _______/
                  i
            """

        And the ways
            | nodes | highway        | oneway | name  |
            | ab    | secondary      | yes    | road  |
            | be    | secondary      | yes    | road  |
            | ecd   | secondary      | no     | road  |
            | efg   | secondary      | yes    | road  |
            | ehb   | secondary_link | yes    | road  |
            | bi    | tertiary       | no     | cross |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | ehb      | be     | b        | no_left_turn |

        When I route I should get
            | waypoints | route            | turns                   | locations |
            | a,d       | road,road        | depart,arrive           | a,d       |
            | d,i       | road,cross,cross | depart,turn left,arrive | d,r,i     |
            | d,g       | road,road        | depart,arrive           | d,g       |

     Scenario: Go onto turning major road
        Given the node map
            """
                  c


            a     b

                  d
            """

        And the ways
            | nodes | highway     | name | lanes |
            | abc   | primary     | road |     3 |
            | bd    | residential | in   |     1 |

        When I route I should get
            | waypoints | turns                   | route        | locations |
            | a,c       | depart,arrive           | road,road    | a,c       |
            | d,a       | depart,turn left,arrive | in,road,road | d,?,a     |
            | d,c       | depart,arrive           | in,road      | d,c       |

    Scenario: Channing Street
        Given the node map
            """
                g f
                | |
            d---c-b-a
                | |
                | |
                h e
            """

        And the nodes
            | node | highway         |
            | c    | traffic_signals |
            | b    | traffic_signals |

        And the ways
            | nodes | name                           | highway     | oneway |
            | ab    | Channing Street Northeast      | residential | no     |
            | bcd   | Channing Street Northwest      | residential | yes    |
            | ebf   | North Capitol Street Northeast | primary     | yes    |
            | gch   | North Capitol Street Northeast | primary     | yes    |

        When I route I should get
            | waypoints | turns                   | route                                                                                   | locations |
            | a,d       | depart,arrive           | Channing Street Northeast,Channing Street Northwest                                     | a,d       |
            | a,h       | depart,turn left,arrive | Channing Street Northeast,North Capitol Street Northeast,North Capitol Street Northeast | a,t,h     |

    Scenario: V St NW, Florida Ave NW: Turn Instruction
    # https://www.mapillary.com/app/?focus=map&lat=38.91815595&lng=-77.03880249&z=17&pKey=sCxepTOCTZD3OoBXuqGEOw
    # http://www.openstreetmap.org/way/6062557#map=19/38.91805/-77.03892
        Given the node map
            """
            y     x
                c
              d     b a

            e
            """

        And the ways
            | nodes | name                           | highway     | oneway |
            | abc   | V St NW                        | tertiary    | yes    |
            | xcde  | Florida Ave NW                 | tertiary    | yes    |
            | yd    | Champlain St NW                | residential |        |

        When I route I should get
            | waypoints | turns                   | route                                 | locations |
            | a,e       | depart,turn left,arrive | V St NW,Florida Ave NW,Florida Ave NW | a,W,e     |

    # http://www.openstreetmap.org/node/182805179
    Scenario: Make Sharp Left at Traffic Signal
        Given the node map
            """
                  g
                  |
               _--f-----y
            i-'   |
            j-k-a]|[b---x
                  e  'c
                  |'d'
                  |
                  h
                  |
                  q
            """

        And the nodes
            | node | highway         |
            | f    | traffic_signals |

        And the ways
            | nodes | name                           | highway     | oneway |
            | yf    | yf                             | trunk_link  | yes    |
            | gfehq | Centreville Road               | primary     |        |
            | fi    | fi                             | trunk_link  | yes    |
            | ij    | Bloomingdale Road              | residential |        |
            | jkabx | Blue Star Memorial Hwy         | trunk       | yes    |
            | bcde  | bcde                           | trunk_link  | yes    |
            | kh    | kh                             | trunk_link  | yes    |

        When I route I should get
            | waypoints | turns                                        | route                                                         | locations |
            | a,q       | depart,off ramp right,turn sharp left,arrive | Blue Star Memorial Hwy,bcde,Centreville Road,Centreville Road | a,e,e,q   |

    @todo
    # https://www.openstreetmap.org/#map=20/52.51609/13.41080
    Scenario: Unnecessary Slight Left onto Stralauer Strasse
        Given the node map
            """
              e

            a   b   c   d
            """

        And the ways
            | nodes | name          | highway   | oneway |
            | ab    | Molkenmarkt   | secondary | yes    |
            | bc    | Stralauer Str | secondary | yes    |
            | cd    | Stralauer Str | secondary | yes    |
            | ec    | Molkenmarkt   | secondary | yes    |

        When I route I should get
            | waypoints | turns         | route                     | locations |
            | a,d       | depart,arrive | Molkenmarkt,Stralauer Str | a,d       |
            | e,d       | depart,arrive | Molkenmarkt,Stralauer Str | e,d       |

    Scenario: Unnecessary Slight Left onto Stralauer Strasse
        Given the node map
            """
              e

            a   b   c   d
            """

        And the ways
            | nodes | name          | highway   | oneway |
            | ab    | Molkenmarkt   | secondary | yes    |
            | bc    | Molkenmarkt   | secondary | yes    |
            | cd    | Stralauer Str | secondary | yes    |
            | ec    | Molkenmarkt   | secondary | yes    |

        When I route I should get
            | waypoints | turns         | route                     | locations |
            | a,d       | depart,arrive | Molkenmarkt,Stralauer Str | a,d       |
            | e,d       | depart,arrive | Molkenmarkt,Stralauer Str | e,d       |

     # http://www.openstreetmap.org/#map=18/39.28158/-76.62291
     @3002
     Scenario: Obvious Index wigh very narrow turn to the right
        Given the node map
            """
            a - b -.-.- - - c
                       ' ' 'd
            """

        And the ways
            | nodes | highway      | name |
            | abc   | primary      | road |
            | bd    | primary_link |      |

        When I route I should get
            | waypoints | turns                           | route     | locations |
            | a,c       | depart,arrive                   | road,road | a,c       |
            | a,d       | depart,turn slight right,arrive | road,,    | a,?,d     |

     # http://www.openstreetmap.org/#map=18/39.28158/-76.62291
     @3002
     Scenario: Obvious Index wigh very narrow turn to the right
        Given the node map
            """
            a - b - . -.- - c
                    e - -'-'d-f
            """

        And the ways
            | nodes | highway      | name |
            | abc   | primary      | road |
            | bd    | primary_link |      |
            | edf   | primary_link |      |

        When I route I should get
            | waypoints | turns                           | route     | locations |
            | a,c       | depart,arrive                   | road,road | a,c       |
            | a,f       | depart,turn slight right,arrive | road,,    | a,?,f     |

    # http://www.openstreetmap.org/#map=18/39.28158/-76.62291
    @3002
    Scenario: Obvious Index wigh very narrow turn to the left
        Given the node map
            """
                       . . .d
            a - b -'-'- - - c
            """

        And the ways
            | nodes | highway      | name |
            | abc   | primary      | road |
            | bd    | primary_link |      |

        When I route I should get
            | waypoints | turns                          | route     | locations |
            | a,c       | depart,arrive                  | road,road | a,c       |
            | a,d       | depart,turn slight left,arrive | road,,    | a,?,d     |

     # http://www.openstreetmap.org/#map=18/39.28158/-76.62291
     @3002
     Scenario: Obvious Index wigh very narrow turn to the left
        Given the node map
            """
                    e - -.- d-f
            a - b - ' - - - c
            """

        And the ways
            | nodes | highway      | name |
            | abc   | primary      | road |
            | bd    | primary_link |      |
            | edf   | primary_link |      |

        When I route I should get
            | waypoints | turns                          | route     | locations |
            | a,f       | depart,turn slight left,arrive | road,,    | a,?,f     |
            | a,c       | depart,arrive                  | road,road | a,c       |

    Scenario: Non-Obvious Turn Next to service road
        Given the node map
            """
                                c
                               .
                               .
                               .
                               .
                               .
                             .
                             .
                             .
                             .
                             .
            a - - - - - - - b - - - d
                            |
                            |
                            |
                            |
                            |
                            |
                            |
                            e
            """

        And the ways
            | nodes  | highway | name    |
            | ab     | primary | in      |
            | bc     | primary | through |
            | be     | primary | through |
            | bd     | service |         |

       When I route I should get
            | waypoints | route              | turns                   | locations |
            | a,c       | in,through,through | depart,turn left,arrive | a,?,c     |

    # http://www.openstreetmap.org/#map=19/52.51556/13.41832
    Scenario: No Slight Right at Stralauer Strasse
        Given the node map
        """
                  l   m
                  |   |
            f._   |   |
                ' g---h.
                  |   |  '-i
                  |   |
            a_    |   |
               ''.b---c
                  |   |' d._
                  |   |     'e
                  j   k
        """

        And the ways
            | nodes | name          | highway   | oneway |
            | ab    | Stralauer Str | tertiary  | yes    |
            | bcde  | Holzmarktstr  | secondary | yes    |
            | gf    | Stralauer Str | tertiary  | yes    |
            | ihg   | Holzmarktstr  | secondary | yes    |
            | lgbj  | Alexanderstr  | primary   | yes    |
            | kchm  | Alexanderstr  | primary   | yes    |

        When I route I should get
            | waypoints | turns         | route                      | locations |
            | a,e       | depart,arrive | Stralauer Str,Holzmarktstr | a,e       |

    Scenario: No Slight Right at Stralauer Strasse -- less extreme
        Given the node map
         """
                  l   m
                  |   |
            f_    |   |
               ' 'g---h_
                  |   |  '\_
                  |   |     i
            a_    |   |
               '_ b___c_
                  |   |  \_
                  |   |     e
                  j   k
         """

        And the ways
            | nodes | name          | highway   | oneway |
            | ab    | Stralauer Str | tertiary  | yes    |
            | bce   | Holzmarktstr  | secondary | yes    |
            | gf    | Stralauer Str | tertiary  | yes    |
            | ihg   | Holzmarktstr  | secondary | yes    |
            | lgbj  | Alexanderstr  | primary   | yes    |
            | kchm  | Alexanderstr  | primary   | yes    |

        When I route I should get
            | waypoints | turns         | route                      | locations |
            | a,e       | depart,arrive | Stralauer Str,Holzmarktstr | a,e       |

    Scenario: No Slight Right at Stralauer Strasse
        Given the node map
         """
                  l   m
                  |   |
                  |   |
              _ _ g---h_
            f'    |   |  '_
                  |   |     i
                  |   |
               _ _b---c__
            a'    |   |    'd
                  |   |
                  j   k
         """

        And the ways
            | nodes | name          | highway   | oneway |
            | ab    | Stralauer Str | tertiary  | yes    |
            | bcd   | Holzmarktstr  | secondary | yes    |
            | gf    | Stralauer Str | tertiary  | yes    |
            | ihg   | Holzmarktstr  | secondary | yes    |
            | lgbj  | Alexanderstr  | primary   | yes    |
            | kchm  | Alexanderstr  | primary   | yes    |

        When I route I should get
            | waypoints | turns         | route                      | locations |
            | a,d       | depart,arrive | Stralauer Str,Holzmarktstr | a,d       |

    #http://www.openstreetmap.org/#map=19/49.48761/8.47618
    @todo @3365
    Scenario: Turning Road - Segregated
        Given the node map
            """
                    f   d
                    |   |
            a - - - b - c
                    |   |
                    |   |
                    g   e
            """
        And the ways
            | nodes | name   | ref  | oneway |
            | ab    | Goethe | B 38 | yes    |
            | bc    |        | B 38 | yes    |
            | ec    | Fried  |      | yes    |
            | cd    | Fried  | B 38 | yes    |
            | fbg   | Fried  |      | yes    |

        When I route I should get
            | waypoints | route              | turns                       | locations |
            | a,d       | Goethe,Fried,Fried | depart,continue left,arrive | a,e,d     |
            | a,g       | Goethe,Fried,Fried | depart,turn right,arrive    | a,e,g     |

	# Conflicting roads (https://www.openstreetmap.org/export#map=19/37.57805/-77.46049)
	Scenario: Turning at forklike structure
        Given the node map
            """
            c  d
               - - - b - - - a
                   -
              e
            """
        And the ways
            | nodes | name | oneway | highway       |
            | abc   | foo  | no     | residential   |
            | bd    | bar  | yes    | residential   |
            | eb    | some | yes    | tertiary_link |

        When I route I should get
            | waypoints | route       | turns                           | locations |
            | a,d       | foo,bar,bar | depart,turn slight right,arrive | a,?,d     |

    Scenario: UTurn onto ramp
        Given the node map
            """
                       a - - - b - c
                                  .|
                _________________ de
            h-g-----------------------f
            """
        And the ways
            | nodes | name  | ref  | oneway | highway       |
            | abc   | Road  |      | yes    | primary       |
            | ce    | other |      | yes    | primary       |
            | cdg   |       |      | yes    | motorway_link |
            | fgh   |       | C 42 | yes    | motorway      |


        When I route I should get
            | waypoints | route   | ref         | turns                                         | locations |
            | a,h       | Road,,, | ,,C 42,C 42 | depart,on ramp right,merge slight left,arrive | a,?,?,h   |

    Scenario: UTurn onto ramp (same ref)
        Given the node map
            """
                       a - - - b - c
                                  .|
                _________________ de
            h-g-----------------------f
            """
        And the ways
            | nodes | name  | ref  | oneway | highway       |
            | abc   | Road  | C 42 | yes    | primary       |
            | ce    | other |      | yes    | primary       |
            | cdg   |       |      | yes    | motorway_link |
            | fgh   |       | C 42 | yes    | motorway      |


        When I route I should get
            | waypoints | route   | ref             | turns                                         | locations |
            | a,h       | Road,,, | C 42,,C 42,C 42 | depart,on ramp right,merge slight left,arrive | a,?,?,h   |

    Scenario: End of road, T-intersection, no obvious turn, only one road allowed
        Given the node map
            """
                           d
                          .
            a . b  .  .  c
                    '   .
                      'e
                      .
                      f
            """

        And the ways
            | nodes  | highway      | oneway | ref       |
            | ab     | primary      |        | B 191     |
            | bc     | primary      |        | B 191     |
            | be     | primary_link | yes    |           |
            | dc     | primary      |        | B 4;B 191 |
            | ce     | primary      |        | B 4       |
            | ef     | primary      |        | B 4       |

        And the relations
            | type        | way:from | way:to | node:via | restriction     |
            | restriction | bc       | ce     | c        | no_right_turn   |
            | restriction | be       | ef     | e        | only_right_turn |

       When I route I should get
            | waypoints | route    | turns                   | locations |
            | a,d       | ab,dc,dc | depart,turn left,arrive | a,?,d     |


    # https://www.openstreetmap.org/node/1332083066
    Scenario: Turns ordering must respect initial bearings
        Given the node map
            """
            a . be .
                  \ c.
                 d/    .f . g
            """

        And the ways
            | nodes | highway | oneway |
            | ab    | primary | yes    |
            | bcd   | primary | yes    |
            | befg  | primary | yes    |

       When I route I should get
            | waypoints | route        | turns                           | locations |
            | a,d       | ab,bcd,bcd   | depart,fork slight right,arrive | a,b,d     |
            | a,g       | ab,befg,befg | depart,fork slight left,arrive  | a,b,g     |

	@routing @car
    Scenario: No turn instruction when turning from unnamed onto unnamed
        Given the node map
            """
            a
            |
            |
            |
            |
            b----------------c
            |
            |
            |
            |
            |
            |
            d
            """

        And the ways
            | nodes | highway     | name | ref   |
            | ab    | trunk_link  |      |       |
            | db    | secondary   |      | L 460 |
            | bc    | secondary   |      |       |

        When I route I should get
            | from | to | route | ref          | turns                    | locations |
            | d    | c  | ,,    | L 460,,      | depart,turn right,arrive | d,?,c     |
            | c    | d  | ,,    | ,L 460,L 460 | depart,turn left,arrive  | c,?,d     |

    # https://www.openstreetmap.org/#map=18/52.25130/10.42545
    Scenario: Turn for roads with no name, ref changes
        Given the node map
            """
              d
              .
              .
            e c . . f
              .
              .
              b
              .
              .
              a
            """

        And the ways
            | nodes | highway     | ref  | name          |
            | abc   | tertiary    | K 57 |               |
            | cd    | tertiary    | K 56 |               |
            | cf    | tertiary    | K 56 |               |
            | ce    | residential |      | Heinrichshöhe |

       When I route I should get
            | waypoints | route | turns                    | locations |
            | a,f       | ,,    | depart,turn right,arrive | a,?,f     |

    # https://www.openstreetmap.org/#map=18/52.24071/10.29066
    Scenario: Turn for roads with no name, ref changes
        Given the node map
            """
                     x
                     .
                     .
                     d
                    . .
                   .   .
                  .     .
            e. . t . c . p. .f
                  .     .
                   .   .
                    . .
                     b
                     .
                     .
                     a
            """

        And the ways
            | nodes | highway     | ref  | name          | oneway |
            | abp   | tertiary    | K 23 |               | yes    |
            | pdx   | tertiary    | K 23 |               | yes    |
            | xdt   | tertiary    | K 23 |               | yes    |
            | tba   | tertiary    | K 23 |               | yes    |
            | etcpf | primary     | B 1  |               | no     |

       When I route I should get
            | waypoints | route | turns                   | locations |
            | e,x       | ,,    | depart,turn left,arrive | e,?,x     |
            | f,a       | ,,    | depart,turn left,arrive | f,?,a     |
