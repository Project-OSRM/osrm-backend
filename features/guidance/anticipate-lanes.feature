@routing @guidance @turn-lanes
Feature: Turn Lane Guidance

    Background:
        Given the profile "car"
        Given a grid size of 20 meters

    @anticipate
    Scenario: Anticipate Lane Change for subsequent multi-lane intersections
        Given the node map
            """
            a – b – x
                |
                c – d – z
                |   |
                y   e
            """

        And the ways
            | nodes | turn:lanes:forward           |
            | ab    | through\|right\|right\|right |
            | bx    |                              |
            | bc    | left\|left\|through          |
            | cd    | through\|right               |
            | cy    |                              |
            | dz    |                              |
            | de    |                              |

       When I route I should get
            | waypoints | route          | turns                                         | lanes                                                                                                             | #      |
            | a,d       | ab,bc,cd,cd    | depart,turn right,turn left,arrive            | ,straight:false right:true right:true right:false,left:true left:true straight:false,                             | 2 hops |
            | a,e       | ab,bc,cd,de,de | depart,turn right,turn left,turn right,arrive | ,straight:false right:false right:true right:false,left:false left:true straight:false,straight:false right:true, | 3 hops |

    @anticipate
    Scenario: Anticipate Lane Change for quick same direction turns, staying on the same street
        Given the node map
            """
            a – b – x
                |
                c
                |
            e – d – y
            """

        And the ways
            | nodes | turn:lanes:forward    | turn:lanes:backward | name |
            | ab    | through\|right\|right |                     | MySt |
            | bx    |                       |                     | XSt  |
            | bc    |                       | left\|right         | MySt |
            | cd    | left\|right           | through\|through    | MySt |
            | de    |                       | left\|left\|through | MySt |
            | dy    |                       |                     | YSt  |

       When I route I should get
            | waypoints | route               | turns                                   | lanes                                                         |
            | a,e       | MySt,MySt,MySt,MySt | depart,continue right,turn right,arrive | ,straight:false right:false right:true,left:false right:true, |
            | e,a       | MySt,MySt,MySt,MySt | depart,continue left,turn left,arrive   | ,left:true left:false straight:false,left:true right:false,   |

    @anticipate
    Scenario: Anticipate Lane Change for quick same direction turns, changing between streets
        Given the node map
            """
            a – b – x
                |
                c
                |
            e – d – y
            """

        And the ways
            | nodes | turn:lanes:forward    | turn:lanes:backward | name |
            | ab    | through\|right\|right |                     | AXSt |
            | bx    |                       |                     | AXSt |
            | bc    |                       | left\|right         | BDSt |
            | cd    | left\|right           | through\|through    | BDSt |
            | de    |                       | left\|left\|through | EYSt |
            | dy    |                       |                     | EYSt |

       When I route I should get
            | waypoints | route               | turns                               | lanes                                                         |
            | a,e       | AXSt,BDSt,EYSt,EYSt | depart,turn right,turn right,arrive | ,straight:false right:false right:true,left:false right:true, |
            | e,a       | EYSt,BDSt,AXSt,AXSt | depart,turn left,turn left,arrive   | ,left:true left:false straight:false,left:true right:false,   |


    @anticipate
    Scenario: Anticipate Lane Change for quick turns during a merge
        Given the node map
            """
            a
              \
            x – b – c – y
                    |
                    d
            """

        And the ways
            | nodes | turn:lanes:forward       | name | highway       | oneway |
            | ab    | slight_left\|slight_left | On   | motorway_link | yes    |
            | xb    |                          | Hwy  | motorway      |        |
            | bc    | through\|slight_right    | Hwy  | motorway      |        |
            | cd    |                          | Off  | motorway_link | yes    |
            | cy    |                          | Hwy  | motorway      |        |

       When I route I should get
            | waypoints | route          | turns                                           | lanes                                                                 |
            | a,d       | On,Hwy,Off,Off | depart,merge slight right,off ramp right,arrive | ,slight left:false slight left:true,straight:false slight right:true, |


    @anticipate
    Scenario: Schoenefelder Kreuz
    # https://www.openstreetmap.org/way/264306388#map=16/52.3202/13.5568
        Given the node map
            """
            a b – x
                \       / i
                  c – d
                        \ j
            """

        And the ways
            | nodes | turn:lanes:forward                                  | lanes | highway       | oneway | name |
            | ab    | none\|none\|none\|slight_right\|slight_right        |   5   | motorway      |        | abx  |
            | bx    |                                                     |   3   | motorway      |        | abx  |
            | bc    |                                                     |   2   | motorway_link | yes    | bcd  |
            | cd    | slight_left\|slight_left;slight_right\|slight_right |   3   | motorway_link | yes    | bcd  |
            | di    | slight_left\|slight_right                           |   2   | motorway_link | yes    | di   |
            | dj    |                                                     |   2   | motorway_link | yes    | dj   |

       When I route I should get
            | waypoints | route         | turns                                          | lanes                                                                                                                                    |
            | a,i       | abx,bcd,di,di | depart,off ramp right,fork slight left,arrive  | ,none:false none:false none:false slight right:true slight right:true,slight left:true slight left;slight right:true slight right:false, |
            | a,j       | abx,bcd,dj,dj | depart,off ramp right,fork slight right,arrive | ,none:false none:false none:false slight right:true slight right:true,slight left:false slight left;slight right:true slight right:true, |


    @anticipate
    Scenario: Kreuz Oranienburg
    # https://www.openstreetmap.org/way/4484007#map=18/52.70439/13.20269
        Given the node map
            """
            i               a
              ' .       . '
            j – – c – b – – x
            """

        And the ways
            | nodes | turn:lanes:forward | lanes | highway       | oneway | name |
            | ab    |                    | 1     | motorway_link | yes    | ab   |
            | xb    |                    | 1     | motorway_link | yes    | xbcj |
            | bc    | none\|slight_right | 2     | motorway_link | yes    | xbcj |
            | ci    |                    | 1     | motorway_link | yes    | ci   |
            | cj    |                    | 1     | motorway_link | yes    | xbcj |

       When I route I should get
            | waypoints | route        | turns                           | lanes                          |
            | a,i       | ab,ci,ci     | depart,turn slight right,arrive | ,none:false slight right:true, |
            | a,j       | ab,xbcj      | depart,arrive                   | ,                              |


    @anticipate
    Scenario: Lane anticipation for fan-in
        Given the node map
            """
            a – b – x
                |
                c – d – z
                |   |
                y   e
            """

        And the ways
            | nodes | turn:lanes:forward           | name |
            | ab    | through\|right\|right\|right | abx  |
            | bx    |                              | abx  |
            | bc    | left\|left\|through          | bcy  |
            | cy    |                              | bcy  |
            | cd    | through\|right               | cdz  |
            | dz    |                              | cdz  |
            | de    |                              | de   |

       When I route I should get
            | waypoints | route             | turns                                         | lanes                                                                                                             |
            | a,e       | abx,bcy,cdz,de,de | depart,turn right,turn left,turn right,arrive | ,straight:false right:false right:true right:false,left:false left:true straight:false,straight:false right:true, |

    @anticipate
    Scenario: Lane anticipation for fan-out
        Given the node map
            """
            a – b – x
                |
                c – d – z
                |   |
                y   e
            """

        And the ways
            | nodes | turn:lanes:forward           | name |
            | ab    | through\|right               | abx  |
            | bx    |                              | abx  |
            | bc    | left\|left\|through          | bcy  |
            | cy    |                              | bcy  |
            | cd    | through\|right\|right\|right | cdz  |
            | dz    |                              | cdz  |
            | de    |                              | de   |

       When I route I should get
            | waypoints | route             | turns                                         | lanes                                                                                                          |
            | a,e       | abx,bcy,cdz,de,de | depart,turn right,turn left,turn right,arrive | ,straight:false right:true,left:true left:true straight:false,straight:false right:true right:true right:true, |

    @anticipate
    Scenario: Lane anticipation for fan-in followed by fan-out
        Given the node map
            """
            a – b – x
                |
                c – d – z
                |   |
                y   e
            """

        And the ways
            | nodes | turn:lanes:forward           | name |
            | ab    | through\|right\|right\|right | abx  |
            | bx    |                              | abx  |
            | bc    | left\|left\|through          | bcy  |
            | cy    |                              | bcy  |
            | cd    | through\|right\|right\|right | cdz  |
            | dz    |                              | cdz  |
            | de    |                              | de   |

       When I route I should get
            | waypoints | route             | turns                                         | lanes                                                                                                                                 |
            | a,e       | abx,bcy,cdz,de,de | depart,turn right,turn left,turn right,arrive | ,straight:false right:true right:true right:false,left:true left:true straight:false,straight:false right:true right:true right:true, |

    @anticipate
    Scenario: Lane anticipation for fan-out followed by fan-in
        Given the node map
            """
            a – b – x
                |
                c – d – z
                |   |
                y   e
            """

        And the ways
            | nodes | turn:lanes:forward  | name |
            | ab    | through\|right      | abx  |
            | bx    |                     | abx  |
            | bc    | left\|left\|through | bcy  |
            | cy    |                     | bcy  |
            | cd    | through\|right      | cdz  |
            | dz    |                     | cdz  |
            | de    |                     | de   |

       When I route I should get
            | waypoints | route             | turns                                         | lanes                                                                                     |
            | a,e       | abx,bcy,cdz,de,de | depart,turn right,turn left,turn right,arrive | ,straight:false right:true,left:false left:true straight:false,straight:false right:true, |

    @anticipate
    Scenario: Lane anticipation for multiple hops with same number of lanes
        Given the node map
            """
            a – b – x
                |
                c – d – z
                |   |
                y   e – f
                    |
                    w
            """

        And the ways
            | nodes | turn:lanes:forward           | name |
            | ab    | through\|right\|right\|right | abx  |
            | bx    |                              | abx  |
            | bc    | left\|left\|through          | bcy  |
            | cy    |                              | bcy  |
            | cd    | through\|right\|right        | cdz  |
            | dz    |                              | cdz  |
            | de    | left\|through                | dew  |
            | ew    |                              | dew  |
            | ef    |                              | ef   |

       When I route I should get
            | waypoints | route                 | turns                                                   | lanes                                                                                                                                                  |
            | a,f       | abx,bcy,cdz,dew,ef,ef | depart,turn right,turn left,turn right,turn left,arrive | ,straight:false right:true right:false right:false,left:true left:false straight:false,straight:false right:true right:false,left:true straight:false, |

       @anticipate
       Scenario: Anticipate Lanes for through, through with lanes
           Given the node map
               """
                         f   g
                        /   /
               a – b – c – d – e
                        \   \
                         h   i
               """

           And the ways
               | nodes | turn:lanes:forward                     | name |
               | ab    |                                        | main |
               | bc    | left\|through\|through\|through\|right | main |
               | cd    | left\|through\|right                   | main |
               | de    |                                        | main |
               | cf    |                                        | off  |
               | ch    |                                        | off  |
               | dg    |                                        | off  |
               | di    |                                        | off  |

          When I route I should get
               | waypoints | route          | turns                           | lanes                                                                |
               | a,e       | main,main,main | depart,use lane straight,arrive | ,left:false straight:false straight:true straight:false right:false, |

       @anticipate
       Scenario: Anticipate Lanes for through and collapse multiple use lanes
           Given the node map
               """
                     e   f   g
                    /   /   /
               a – b – c – d
                    \   \   \
                     h   i   j
               """

           And the ways
               | nodes | turn:lanes:forward                     | name |
               | ab    | left\|through\|through\|right          | main |
               | bc    | left\|through\|through\|right          | main |
               | cd    | left\|through\|through\|through\|right | main |
               | be    |                                        | off  |
               | bh    |                                        | off  |
               | cf    |                                        | off  |
               | ci    |                                        | off  |
               | dg    |                                        | off  |
               | dj    |                                        | off  |

          When I route I should get
               | waypoints | route     | turns         | lanes |
               | a,c       | main,main | depart,arrive | ,     |
               | a,d       | main,main | depart,arrive | ,     |

       @anticipate
       Scenario: Anticipate Lanes for through followed by left/right
           Given the node map
               """
                     f   g   d
                    /   /   /
               a – b – c – x
                    \   \   \
                     h   i   e
               """

           And the ways
               | nodes | turn:lanes:forward                              | name  |
               | ab    | left\|through\|through\|through\|through\|right | main  |
               | bc    | left\|through\|through\|right                   | main  |
               | cx    | left\|right                                     | main  |
               | xd    |                                                 | left  |
               | xe    |                                                 | right |
               | bf    |                                                 | off   |
               | bh    |                                                 | off   |
               | cg    |                                                 | off   |
               | ci    |                                                 | off   |

          When I route I should get
               | waypoints | route                      | turns                                                        | lanes                                                                                                                                                         |
               | a,d       | main,main,main,left,left   | depart,use lane straight,use lane straight,turn left,arrive  | ,left:false straight:false straight:true straight:false straight:false right:false,left:false straight:true straight:false right:false,left:true right:false, |
               | a,e       | main,main,main,right,right | depart,use lane straight,use lane straight,turn right,arrive | ,left:false straight:false straight:false straight:true straight:false right:false,left:false straight:false straight:true right:false,left:false right:true, |

       @anticipate
       Scenario: Anticipate Lanes for through with turn before / after
           Given the node map
               """
               c       g       l
               b – d – e – h - i
               a       f       j
               """

           And the ways
               | nodes | turn:lanes:forward                                           | name  | oneway |
               | ab    | right\|right\|right\|right                                   | ab    | yes    |
               | cb    | left\|left\|left\|left                                       | cb    | yes    |
               | bd    |                                                              | bdehi |        |
               | de    | left\|left\|through\|through\|through\|through\|right\|right | bdehi |        |
               | ef    |                                                              | ef    |        |
               | eg    |                                                              | eg    |        |
               | eh    |                                                              | bdehi |        |
               | hi    | left\|left\|right\|right                                     | bdehi |        |
               | ij    |                                                              | ij    |        |
               | il    |                                                              | il    |        |

          When I route I should get
               | waypoints | route                | turns                                                 | lanes                                                                                                                                                                                               | #           |
               | a,f       | ab,bdehi,ef,ef       | depart,turn right,turn right,arrive                   | ,right:false right:false right:true right:true,left:false left:false straight:false straight:false straight:false straight:false right:true right:true,                                             |             |
               | a,g       | ab,bdehi,eg,eg       | depart,turn right,turn left,arrive                    | ,right:true right:true right:false right:false,left:true left:true straight:false straight:false straight:false straight:false right:false right:false,                                             |             |
               | a,j       | ab,bdehi,bdehi,ij,ij | depart,turn right,use lane straight,turn right,arrive | ,right:true right:true right:false right:false,left:false left:false straight:false straight:false straight:true straight:true right:false right:false,left:false left:false right:true right:true, |             |
               | a,l       | ab,bdehi,bdehi,il,il | depart,turn right,use lane straight,turn left,arrive  | ,right:false right:false right:true right:true,left:false left:false straight:true straight:true straight:false straight:false right:false right:false,left:true left:true right:false right:false, | not perfect |
               | c,g       | cb,bdehi,eg,eg       | depart,turn left,turn left,arrive                     | ,left:true left:true left:false left:false,left:true left:true straight:false straight:false straight:false straight:false right:false right:false,                                                 |             |
               | c,f       | cb,bdehi,ef,ef       | depart,turn left,turn right,arrive                    | ,left:false left:false left:true left:true,left:false left:false straight:false straight:false straight:false straight:false right:true right:true,                                                 |             |
               | c,l       | cb,bdehi,bdehi,il,il | depart,turn left,use lane straight,turn left,arrive   | ,left:false left:false left:true left:true,left:false left:false straight:true straight:true straight:false straight:false right:false right:false,left:true left:true right:false right:false,     |             |
               | c,j       | cb,bdehi,bdehi,ij,ij | depart,turn left,use lane straight,turn right,arrive  | ,left:true left:true left:false left:false,left:false left:false straight:false straight:false straight:true straight:true right:false right:false,left:false left:false right:true right:true,     | not perfect |

       @anticipate
       Scenario: Anticipate Lanes for turns with through before and after
           Given a grid size of 10 meters
           Given the node map
               """
               a – b – q       s   h – i
                     \       /   /
                       e – f – g
                     /       \   \
               c – d – r       t   j – k

               """

           And the ways
               | nodes | turn:lanes:forward                              | name | highway | oneway |
               | ab    | through\|right\|right\|right                    | top  | primary | yes    |
               | be    |                                                 | top  | primary | yes    |
               | bq    |                                                 | off  | primary | yes    |
               | ef    | left\|through\|through\|through\|through\|right | main | primary | yes    |
               | fg    | left\|left\|right\|right                        | main | primary | yes    |
               | fs    |                                                 | off  | primary | yes    |
               | ft    |                                                 | off  | primary | yes    |
               | gh    |                                                 | top  | primary | yes    |
               | hi    |                                                 | top  | primary | yes    |
               | cd    | left\|left\|left\|through                       | bot  | primary | yes    |
               | de    |                                                 | bot  | primary | yes    |
               | dr    |                                                 | off  | primary | yes    |
               | gj    |                                                 | bot  | primary | yes    |
               | jk    |                                                 | bot  | primary | yes    |

          When I route I should get
               | waypoints | route                 | turns                                                  | lanes                                                                                                                                                                           |
               | a,i       | top,main,main,top,top | depart,turn right,use lane straight,turn left,arrive   | ,straight:false right:false right:true right:true,left:false straight:true straight:true straight:false straight:false right:false,left:true left:true right:false right:false, |
               | a,k       | top,main,main,bot,bot | depart,turn right,use lane straight,turn right,arrive  | ,straight:false right:true right:true right:false,left:false straight:false straight:false straight:true straight:true right:false,left:false left:false right:true right:true, |
               | c,i       | bot,main,main,top,top | depart,turn left,use lane straight,turn left,arrive    | ,left:false left:true left:true straight:false,left:false straight:true straight:true straight:false straight:false right:false,left:true left:true right:false right:false,    |
               | c,k       | bot,main,main,bot,bot | depart,turn left,use lane straight,turn right,arrive   | ,left:true left:true left:false straight:false,left:false straight:false straight:false straight:true straight:true right:false,left:false left:false right:true right:true,    |

       @anticipate
       Scenario: Anticipate Lanes for turn between throughs
           Given the node map
               """
                   q
                   |
               a – b – c – s
                   |   |
                   r   d – t
                       |
                       e
               """

           And the ways
               | nodes | turn:lanes:forward                                       | name |
               | ab    | left\|through\|through\|through\|through\|through\|right | main |
               | bq    |                                                          | off  |
               | br    |                                                          | off  |
               | bc    | through\|through\|right\|right\|right                    | main |
               | cs    |                                                          | off  |
               | cd    | left\|through\|through                                   | main |
               | de    |                                                          | main |
               | dt    |                                                          | off  |

          When I route I should get
               | waypoints | route               | turns                                          | lanes                                                                                                                                                             |
               | a,e       | main,main,main,main | depart,use lane straight,continue right,arrive | ,left:false straight:false straight:false straight:false straight:true straight:true right:false,straight:false straight:false right:false right:true right:true, |

    @anticipate @todo @2661
    Scenario: Anticipate with lanes in roundabout: roundabouts as the unit of anticipation
        Given the node map
            """
                 /e\
            a – b   d – f
                 \c/
                  |
                 /g\
            k – h   j – l
                 \i/
            """

        And the ways
            | nodes | turn:lanes:forward                       | highway | junction   | #   |
            | ab    | slight_right\|slight_right\|slight_right | primary |            |     |
            | bc    | slight_left\|slight_right\|slight_right  | primary | roundabout | top |
            | cd    |                                          | primary | roundabout | top |
            | de    |                                          | primary | roundabout | top |
            | eb    |                                          | primary | roundabout | top |
            | df    |                                          | primary |            |     |
            | cg    | slight_right\|slight_right               | primary |            |     |
            | gh    | slight_left\|slight_right                | primary | roundabout | bot |
            | hi    |                                          | primary | roundabout | bot |
            | ij    | slight_left\|slight_right                | primary | roundabout | bot |
            | jg    |                                          | primary | roundabout | bot |
            | hk    |                                          | primary |            |     |
            | jl    |                                          | primary |            |     |

        When I route I should get
            | #           | waypoints | route       | turns                                             | lanes                                                                                          |
            | right-right | a,k       | ab,cg,hk,hk | depart,roundabout-exit-1,roundabout-exit-1,arrive | ,slight right:false slight right:false slight right:true,slight right:false slight right:true, |
            | right-left  | a,l       | ab,cg,jl,jl | depart,roundabout-exit-1,roundabout-exit-2,arrive | ,slight right:false slight right:false slight right:true,slight right:false slight right:true, |
            | todo exits  | a,f       | ab,df,df    | depart,roundabout-exit-2,arrive                   | ,slight right:false slight right:false slight right:true,                                      |
            | todo exits  | a,e       | ab,bc,eb    | depart,roundabout-exit-undefined,arrive           | ,slight right:true slight right:true slight right:true,                                        |

    @anticipate @todo
    Scenario: Roundabout with lanes only tagged on exit
        Given the node map
            """
                 /e\
            a – b   d – f
                 \c/
            """

        And the ways
            | nodes | turn:lanes:forward                     | highway | junction   |
            | ab    |                                        | primary |            |
            | bc    |                                        | primary | roundabout |
            | cd    | slight_left\|slight_left\|slight_right | primary | roundabout |
            | de    |                                        | primary | roundabout |
            | eb    |                                        | primary | roundabout |
            | df    |                                        | primary |            |

        When I route I should get
            | waypoints | route    | turns                                                 | lanes                                                    |
            | a,f       | ab,df,df | depart,roundabout-exit-1,use lane slight right,arrive | ,,slight left:false slight left:false slight right:true, |

    @anticipate
    Scenario: No Lanes for Roundabouts, see #2626
        Given the node map
            """
                a
                |
               /b\
              c   g – h
             /|   |
            | d   f
            |/ \e/ \
            x     \ y
            """

        And the ways
            | nodes | turn:lanes:forward         | highway | junction   |
            | ab    | slight_right\|slight_right | primary |            |
            | bc    |                            | primary | roundabout |
            | cd    |                            | primary | roundabout |
            | de    |                            | primary | roundabout |
            | ef    |                            | primary | roundabout |
            | fg    | through\|slight_right      | primary | roundabout |
            | gb    |                            | primary | roundabout |
            | gh    |                            | primary |            |
            | cx    |                            | primary |            |
            | dx    |                            | primary |            |
            | ey    |                            | primary |            |
            | fy    |                            | primary |            |

        When I route I should get
            | waypoints | route    | turns                           | lanes |
            | a,h       | ab,gh,gh | depart,roundabout-exit-5,arrive | ,,    |

    @anticipate
    Scenario: No Lanes for Roundabouts, see #2626
        Given the node map
            """
                 /a\
            x – b   d – y
                 \c/
            """

        And the ways
            | nodes | turn:lanes:forward         | highway | junction   | name       |
            | xb    | slight_right\|slight_right | primary |            | xb         |
            | dy    |                            | primary |            | dy         |
            | ab    |                            | primary | roundabout | roundabout |
            | bc    |                            | primary | roundabout | roundabout |
            | cd    | left\|slight_right         | primary | roundabout | roundabout |
            | da    |                            | primary | roundabout | roundabout |

        When I route I should get
            | waypoints | route                    | turns                                   | lanes |
            | x,y       | xb,dy,dy                 | depart,roundabout-exit-1,arrive         | ,,    |
            | x,c       | xb,roundabout,roundabout | depart,roundabout-exit-undefined,arrive | ,,    |
            | x,a       | xb,roundabout,roundabout | depart,roundabout-exit-undefined,arrive | ,,    |

    @anticipate
    Scenario: No Lanes for Roundabouts, see #2626
        Given the profile file "car" extended with
        """
        properties.left_hand_driving = true
        """
        And the node map
            """
                  a
                  |
                 /b\
            h – c   g
                |   |\
                d   f |
               / \e/ \|
              x /     y
            """

        And the ways
            | nodes | turn:lanes:forward         | highway | junction   |
            | ab    | slight_left\|slight_left   | primary |            |
            | bg    |                            | primary | roundabout |
            | gf    |                            | primary | roundabout |
            | fe    |                            | primary | roundabout |
            | ed    |                            | primary | roundabout |
            | dc    | slight_left                | primary | roundabout |
            | cb    |                            | primary | roundabout |
            | ch    |                            | primary |            |
            | ex    |                            | primary |            |
            | dx    |                            | primary |            |
            | gy    |                            | primary |            |
            | fy    |                            | primary |            |

        When I route I should get
            | waypoints | route      | turns                           | lanes |
            | a,h       | ab,ch,ch   | depart,roundabout-exit-5,arrive | ,,    |

    @anticipate
    Scenario: No Lanes for Roundabouts, see #2626
        Given the node map
            """
                 /a\
            x – b   d – y
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                 | |
                 | |
                 | |
                 | |
                 | |
                 | |
                 | |
                 | |
                 | |
                 | |
                 | |
                 | |
                  |
                  |
                  |
                  |
                  |
                  |
                  |
                  |
                  |
                  |
                  |
                  |
                  c
            """

        And the ways
            | nodes | turn:lanes:forward         | highway | junction   | name       |
            | xb    | slight_right\|slight_right | primary |            | xb         |
            | dy    |                            | primary |            | dy         |
            | ab    |                            | primary | roundabout | roundabout |
            | bc    |                            | primary | roundabout | roundabout |
            | cd    | left\|slight_right         | primary | roundabout | roundabout |
            | da    |                            | primary | roundabout | roundabout |

        When I route I should get
            | waypoints | route                    | turns                                   | lanes |
            | x,y       | xb,dy,dy                 | depart,roundabout-exit-1,arrive         | ,,    |
            | x,c       | xb,roundabout,roundabout | depart,roundabout-exit-undefined,arrive | ,,    |
            | x,a       | xb,roundabout,roundabout | depart,roundabout-exit-undefined,arrive | ,,    |

    @anticipate @todo @2032
    Scenario: No Lanes for Roundabouts, see #2626
        Given the node map
            """
            a – b –x
                |
               /c\
              d   f – g – z
               \e/    |
                |     h
                y
            """

        And the ways
            | nodes | turn:lanes:forward                                  | highway | junction   | name  |
            | ab    | through\|right\|right\|right\|right                 | primary |            | abx   |
            | bx    |                                                     | primary |            | abx   |
            | bc    | right\|right\|right\|right                          | primary |            | bc    |
            | cd    |                                                     | primary | roundabout | cdefc |
            | de    | slight_left\|slight_left\|slight_left\|slight_right | primary | roundabout | cdefc |
            | ef    | left\|slight_right\|slight_right                    | primary | roundabout | cdefc |
            | fc    |                                                     | primary | roundabout | cdefc |
            | ey    |                                                     | primary |            | ey    |
            | fg    | through\|right                                      | primary |            | fg    |
            | gz    |                                                     | primary |            | gz    |
            | gh    |                                                     | primary |            | gh    |

        When I route I should get
            | waypoints | route           | turns                                            | lanes                                                                                      |
            | a,h       | abx,bc,fg,gh,gh | depart,turn right,cdefc-exit-2,turn right,arrive | ,straight:false right:false right:false right:false right:true,,straight:false right:true, |

    @anticipate
    Scenario: Anticipate none tags
        Given the node map
            """
            c       g       l
            b – d – e – h - i
            a       f       j
            """

        And the ways
            | nodes | turn:lanes:forward       | highway   | name |
            | ab    | none\|none\|right\|right | primary   | abc  |
            | bc    |                          | primary   | abc  |
            | bd    |                          | primary   | bdeh |
            | de    | left\|none\|none\|right  | primary   | bdeh |
            | eh    |                          | primary   | bdeh |
            | ef    |                          | primary   | feg  |
            | eg    |                          | primary   | feg  |

        When I route I should get
            | waypoints | route            | turns                               | lanes                                                                                      |
            | a,g       | abc,bdeh,feg,feg | depart,turn right,turn left,arrive  | ,none:false none:false right:true right:false,left:true none:false none:false right:false, |
            | a,f       | abc,bdeh,feg,feg | depart,turn right,turn right,arrive | ,none:false none:false right:false right:true,left:false none:false none:false right:true, |

    @anticipate
    Scenario: Triple Right keeping Left
        Given the node map
            """
                  a – b – i
                      |
            f – e – g |
                |     |
                |     |
            j – d – – c
                      |
                      h
            """

        And the ways
            | nodes | turn:lanes:forward | highway   | name   |
            | abi   | \|\|right\|right   | primary   | start  |
            | bch   | \|\|right\|right   | primary   | first  |
            | cdj   | \|\|right\|right   | primary   | second |
            | de    | left\|right\|right | secondary | third  |
            | feg   |                    | tertiary  | fourth |

        When I route I should get
            | waypoints | route                                  | turns                                                     | lanes                                                                                                                                                                      |
            | a,f       | start,first,second,third,fourth,fourth | depart,turn right,turn right,turn right,turn left,arrive  | ,none:false none:false right:true right:false,none:false none:false right:true right:false,none:false none:false right:true right:false,left:true right:false right:false, |
            | a,g       | start,first,second,third,fourth,fourth | depart,turn right,turn right,turn right,turn right,arrive | ,none:false none:false right:true right:true,none:false none:false right:true right:true,none:false none:false right:true right:true,left:false right:true right:true,     |

    @anticipate
    Scenario: Tripple Left keeping Right
        Given the node map
            """
            i – b – a
                |
                | g – e – f
                |     |
                |     |
                c – – d – j
                |
                h
            """

        And the ways
            | nodes | turn:lanes:forward | highway   | name   |
            | abi   | left\|left\|\|     | primary   | start  |
            | bch   | left\|left\|\|     | primary   | first  |
            | cdj   | left\|left\|\|     | primary   | second |
            | de    | left\|left\|right  | secondary | third  |
            | feg   |                    | tertiary  | fourth |

        When I route I should get
            | waypoints | route                                  | turns                                                  | lanes                                                                                                                                                               |
            | a,f       | start,first,second,third,fourth,fourth | depart,turn left,turn left,turn left,turn right,arrive | ,left:false left:true none:false none:false,left:false left:true none:false none:false,left:false left:true none:false none:false,left:false left:false right:true, |
            | a,g       | start,first,second,third,fourth,fourth | depart,turn left,turn left,turn left,turn left,arrive  | ,left:true left:true none:false none:false,left:true left:true none:false none:false,left:true left:true none:false none:false,left:true left:true right:false,     |
