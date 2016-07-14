@routing @guidance @turn-lanes
Feature: Turn Lane Guidance

    Background:
        Given the profile "car"
        Given a grid size of 20 meters

    @anticipate
    Scenario: Anticipate Lane Change for subsequent multi-lane intersections
        Given the node map
            | a |   | b |   | x |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d |   | z |
            |   |   |   |   |   |   |   |
            |   |   | y |   | e |   |   |

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
            | a |   | b | x |
            |   |   |   |   |
            |   |   | c |   |
            |   |   |   |   |
            | e |   | d | y |

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
            | a |   | b | x |
            |   |   |   |   |
            |   |   | c |   |
            |   |   |   |   |
            | e |   | d | y |

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
            | a |   |   |   |   |
            | x | b |   | c | y |
            |   |   |   |   | d |

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
            | a | b | x |   |   | i |
            |   |   | c | d |   |   |
            |   |   |   |   |   | j |

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
            | i |   |   |   |   | a |
            | j |   | c | b |   | x |

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
            | a |   | b |   | x |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d |   | z |
            |   |   |   |   |   |   |   |
            |   |   | y |   | e |   |   |

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
            | a |   | b |   | x |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d |   | z |
            |   |   |   |   |   |   |   |
            |   |   | y |   | e |   |   |

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
            | a |   | b |   | x |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d |   | z |
            |   |   |   |   |   |   |   |
            |   |   | y |   | e |   |   |

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
            | a |   | b |   | x |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d |   | z |
            |   |   |   |   |   |   |   |
            |   |   | y |   | e |   |   |

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
            | a |   | b |   | x |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d |   | z |
            |   |   |   |   |   |   |   |
            |   |   | y |   | e |   | f |
            |   |   |   |   |   |   |   |
            |   |   |   |   | w |   |   |

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
            | a,f       | abx,bcy,cdz,dew,ef,ef | depart,turn right,turn left,turn right,turn left,arrive | ,straight:false right:false right:true right:false,left:false left:true straight:false,straight:false right:true right:false,left:true straight:false, |

    @anticipate @todo @bug @2661
    Scenario: Anticipate with lanes in roundabout: roundabouts as the unit of anticipation
        Given the node map
            |   |   | e |   |   |
            | a | b |   | d | f |
            |   |   | c |   |   |
            |   |   |   |   |   |
            |   |   | g |   |   |
            | k | h |   | j | l |
            |   |   | i |   |   |

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
            |   |   | e |   |   |
            | a | b |   | d | f |
            |   |   | c |   |   |

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
            |   |   | a |   |   |
            |   |   | b |   |   |
            |   | c |   | g | h |
            |   |   |   |   |   |
            |   | d |   | f |   |
            |   |   | e |   |   |
            | x |   |   |   | y |

        And the ways
            | nodes | turn:lanes:forward         | highway | junction   |
            | ab    | slight_right\|slight_right | primary |            |
            | bc    |                            | primary | roundabout |
            | cd    |                            | primary | roundabout |
            | de    |                            | primary | roundabout |
            | ef    |                            | primary | roundabout |
            | fg    | slight_right               | primary | roundabout |
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
            |   |   | a |   |   |
            | x | b |   | d | y |
            |   |   | c |   |   |

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
        Given the node map
            |   |   | a |   |   |
            | x | b |   | d | y |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   | c |   |   |

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
        Given the node map
            | a | b |   |   | x |
            |   | c |   |   |   |
            | d |   | f | g | z |
            |   | e |   | h |   |
            |   |   |   |   |   |
            |   | y |   |   |   |

        And the ways
            | nodes | turn:lanes:forward                                 | highway | junction   | name  |
            | ab    | through\|right\|right\|right\|right                | primary |            | abx   |
            | bx    |                                                    | primary |            | abx   |
            | bc    | right\|right\|right\|right                         | primary |            | bc    |
            | cd    |                                                    | primary | roundabout | cdefc |
            | de    | slight_left\|slight_left&slight_left\|slight_right | primary | roundabout | cdefc |
            | ef    | left\|slight_right\|slight_right                   | primary | roundabout | cdefc |
            | fc    |                                                    | primary | roundabout | cdefc |
            | ey    |                                                    | primary |            | ey    |
            | fg    | through\|right                                     | primary |            | fg    |
            | gz    |                                                    | primary |            | gz    |
            | gh    |                                                    | primary |            | gh    |

        When I route I should get
            | waypoints | route           | turns                                            | lanes                                                                                                                                    |
            | a,h       | abx,bc,fg,gh,gh | depart,turn right,cdefc-exit-2,turn right,arrive | ,straight:false right:false right:false right:false right:true,,straight:false right:true, |

    @anticipate @bug @todo
    Scenario: Tripple Right keeping Left
        Given the node map
            | a |   |   |   | b |   | i |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            | f |   | e |   |   |   | g |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   | j | d |   | c |   |   |
            |   |   |   |   | h |   |   |

        And the ways
            | nodes | turn:lanes:forward | highway   | name   |
            | abi   | \|\|right\|right   | primary   | start  |
            | bch   | \|\|right\|right   | primary   | first  |
            | cdj   | \|\|right\|right   | primary   | second |
            | de    | left\|right\|right | secondary | third  |
            | feg   |                    | tertiary  | fourth |

        When I route I should get
            | waypoints | route                                  | turns                                                            | lanes                                                                                                                                                                      |
            | a,f       | start,first,second,third,fourth,fourth | depart,turn right,turn right,turn right,end of road left,arrive  | ,none:false none:true right:false right:false,none:false none:true right:false right:false,none:false none:true right:false right:false,left:true right:false right:false, |
            | a,g       | start,first,second,third,fourth,fourth | depart,turn right,turn right,turn right,end of road right,arrive | ,none:false none:false right:true right:true,none:false none:false right:true right:true,none:false none:false right:true right:true,left:false right:true right:true,     |

    @anticipate @bug @todo
    Scenario: Tripple Left keeping Right
        Given the node map
            | i |   | b |   |   |   | a |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            | g |   |   |   | e |   | f |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d | j |   |
            |   |   | h |   |   |   |   |

        And the ways
            | nodes | turn:lanes:forward | highway   | name   |
            | abi   | left\|left\|\|     | primary   | start  |
            | bch   | left\|left\|\|     | primary   | first  |
            | cdj   | left\|left\|\|     | primary   | second |
            | de    | left\|left\|right  | secondary | third  |
            | feg   |                    | tertiary  | fourth |

        When I route I should get
            | waypoints | route                                  | turns                                                         | lanes                                                                                                                                                               |
            | a,f       | start,first,second,third,fourth,fourth | depart,turn left,turn left,turn left,end of road right,arrive | ,left:false left:false none:true none:false,left:false left:false none:true none:false,left:false left:false none:true none:false,left:false left:false right:true, |
            | a,g       | start,first,second,third,fourth,fourth | depart,turn left,turn left,turn left,end of road left,arrive  | ,left:true left:true none:false none:false,left:true left:true none:false none:false,left:true left:true none:false none:false,left:true left:true right:false,     |
