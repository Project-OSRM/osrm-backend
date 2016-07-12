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
            | nodes | turn:lanes:forward         |
            | ab    | through\|right&right&right |
            | bx    |                            |
            | bc    | left\|left&through         |
            | cd    | through\|right             |
            | cy    |                            |
            | dz    |                            |
            | de    |                            |

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
            | nodes | turn:lanes:forward   | turn:lanes:backward | name |
            | ab    | through\|right&right |                     | MySt |
            | bx    |                      |                     | XSt  |
            | bc    |                      | left\|right         | MySt |
            | cd    | left\|right          | through\|through    | MySt |
            | de    |                      | left\|left&through  | MySt |
            | dy    |                      |                     | YSt  |

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
            | nodes | turn:lanes:forward   | turn:lanes:backward | name |
            | ab    | through\|right&right |                     | AXSt |
            | bx    |                      |                     | AXSt |
            | bc    |                      | left\|right         | BDSt |
            | cd    | left\|right          | through\|through    | BDSt |
            | de    |                      | left\|left&through  | EYSt |
            | dy    |                      |                     | EYSt |

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
            | nodes | turn:lanes:forward                                 | lanes | highway       | oneway | name |
            | ab    | none\|none&none&slight_right&slight_right          |   5   | motorway      |        | abx  |
            | bx    |                                                    |   3   | motorway      |        | abx  |
            | bc    |                                                    |   2   | motorway_link | yes    | bcd  |
            | cd    | slight_left\|slight_left;slight_right&slight_right |   3   | motorway_link | yes    | bcd  |
            | di    | slight_left\|slight_right                          |   2   | motorway_link | yes    | di   |
            | dj    |                                                    |   2   | motorway_link | yes    | dj   |

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
            | waypoints | route             | turns                                             | lanes                           |
            | a,i       | ab,xbcj,ci,ci     | depart,merge slight left,turn slight right,arrive | ,,none:false slight right:true, |
            | a,j       | ab,xbcj,xbcj,xbcj | depart,merge slight left,use lane straight,arrive | ,,none:true slight right:false, |


    @anticipate
    Scenario: Lane anticipation for fan-in
        Given the node map
            | a |   | b |   | x |   |   |
            |   |   |   |   |   |   |   |
            |   |   | c |   | d |   | z |
            |   |   |   |   |   |   |   |
            |   |   | y |   | e |   |   |

        And the ways
            | nodes | turn:lanes:forward         | name |
            | ab    | through\|right&right&right | abx  |
            | bx    |                            | abx  |
            | bc    | left\|left&through         | bcy  |
            | cy    |                            | bcy  |
            | cd    | through\|right             | cdz  |
            | dz    |                            | cdz  |
            | de    |                            | de   |

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
            | nodes | turn:lanes:forward         | name |
            | ab    | through\|right             | abx  |
            | bx    |                            | abx  |
            | bc    | left\|left&through         | bcy  |
            | cy    |                            | bcy  |
            | cd    | through\|right&right&right | cdz  |
            | dz    |                            | cdz  |
            | de    |                            | de   |

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
            | nodes | turn:lanes:forward         | name |
            | ab    | through\|right&right&right | abx  |
            | bx    |                            | abx  |
            | bc    | left\|left&through         | bcy  |
            | cy    |                            | bcy  |
            | cd    | through\|right&right&right | cdz  |
            | dz    |                            | cdz  |
            | de    |                            | de   |

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
            | nodes | turn:lanes:forward         | name |
            | ab    | through\|right             | abx  |
            | bx    |                            | abx  |
            | bc    | left\|left&through         | bcy  |
            | cy    |                            | bcy  |
            | cd    | through\|right             | cdz  |
            | dz    |                            | cdz  |
            | de    |                            | de   |

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
            | nodes | turn:lanes:forward         | name |
            | ab    | through\|right&right&right | abx  |
            | bx    |                            | abx  |
            | bc    | left\|left&through         | bcy  |
            | cy    |                            | bcy  |
            | cd    | through\|right&right       | cdz  |
            | dz    |                            | cdz  |
            | de    | left\|through              | dew  |
            | ew    |                            | dew  |
            | ef    |                            | ef   |

       When I route I should get
            | waypoints | route                 | turns                                                   | lanes                                                                                                                                                  |
            | a,f       | abx,bcy,cdz,dew,ef,ef | depart,turn right,turn left,turn right,turn left,arrive | ,straight:false right:false right:true right:false,left:false left:true straight:false,straight:false right:true right:false,left:true straight:false, |

       @anticipate
       Scenario: Anticipate Lanes for through, through with lanes
           Given the node map
               |   |   |   | f | g |   |
               |   |   |   |   |   |   |
               | a | b | c | d |   | e |
               |   |   |   |   |   |   |
               |   |   |   | h | i |   |

           And the ways
               | nodes | turn:lanes:forward                  | name |
               | ab    |                                     | main |
               | bc    | left\|through&through&through&right | main |
               | cd    | left\|through&right                 | main |
               | de    |                                     | main |
               | cf    |                                     | off  |
               | ch    |                                     | off  |
               | dg    |                                     | off  |
               | di    |                                     | off  |

          When I route I should get
               | waypoints | route               | turns                                             | lanes                                                                                                     |
               | a,e       | main,main,main,main | depart,use lane straight,use lane straight,arrive | ,left:false straight:false straight:false straight:true right:false,left:false straight:true right:false, |

       @anticipate
       Scenario: Anticipate Lanes for through with turn before / after
           Given the node map
               | a | b | c |
               |   | d |   |
               | f | e | g |
               |   | h |   |
               | j | i | l |

           And the ways
               | nodes | turn:lanes:forward                                     | name  | oneway |
               | ab    | right\|right&right&right                               | ab    | yes    |
               | cb    | left\|left&left&left                                   | cb    | yes    |
               | bd    |                                                        | bdehi |        |
               | de    | left\|left&through&through&through&through&right&right | bdehi |        |
               | ef    |                                                        | ef    |        |
               | eg    |                                                        | eg    |        |
               | eh    |                                                        | bdehi |        |
               | hi    | left\|left&right&right                                 | bdehi |        |
               | ij    |                                                        | ij    |        |
               | il    |                                                        | il    |        |

          When I route I should get
               | waypoints | route                | turns                                                 | lanes                                                                                                                                                                                               |
               | a,f       | ab,bdehi,ef,ef       | depart,turn right,turn right,arrive                   | ,right:false right:false right:true right:true,left:false left:false straight:false straight:false straight:false straight:false right:true right:true,                                             |
               | a,g       | ab,bdehi,eg,eg       | depart,turn right,turn left,arrive                    | ,right:true right:true right:false right:false,left:true left:true straight:false straight:false straight:false straight:false right:false right:false,                                             |
               | a,j       | ab,bdehi,bdehi,ij,ij | depart,turn right,use lane straight,turn right,arrive | ,right:false right:false right:true right:true,left:false left:false straight:false straight:false straight:true straight:true right:false right:false,left:false left:false right:true right:true, |
               | a,l       | ab,bdehi,bdehi,il,il | depart,turn right,use lane straight,turn left,arrive  | ,right:true right:true right:false right:false,left:false left:false straight:true straight:true straight:false straight:false right:false right:false,left:true left:true right:false right:false, |
               | c,g       | cb,bdehi,eg,eg       | depart,turn left,turn left,arrive                     | ,left:true left:true left:false left:false,left:true left:true straight:false straight:false straight:false straight:false right:false right:false,                                                 |
               | c,f       | cb,bdehi,ef,ef       | depart,turn left,turn right,arrive                    | ,left:false left:false left:true left:true,left:false left:false straight:false straight:false straight:false straight:false right:true right:true,                                                 |
               | c,l       | cb,bdehi,bdehi,il,il | depart,turn left,use lane straight,turn left,arrive   | ,left:true left:true left:false left:false,left:false left:false straight:true straight:true straight:false straight:false right:false right:false,left:true left:true right:false right:false,     |
               | c,j       | cb,bdehi,bdehi,ij,ij | depart,turn left,use lane straight,turn right,arrive  | ,left:false left:false left:true left:true,left:false left:false straight:false straight:false straight:true straight:true right:false right:false,left:false left:false right:true right:true,     |

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
            | abi   | \|&right&right     | primary   | start  |
            | bch   | \|&right&right     | primary   | first  |
            | cdj   | \|&right&right     | primary   | second |
            | de    | left\|right&right  | secondary | third  |
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
            | abi   | left\|left&&       | primary   | start  |
            | bch   | left\|left&&       | primary   | first  |
            | cdj   | left\|left&&       | primary   | second |
            | de    | left\|left&right   | secondary | third  |
            | feg   |                    | tertiary  | fourth |

        When I route I should get
            | waypoints | route                                  | turns                                                         | lanes                                                                                                                                                               |
            | a,f       | start,first,second,third,fourth,fourth | depart,turn left,turn left,turn left,end of road right,arrive | ,left:false left:false none:true none:false,left:false left:false none:true none:false,left:false left:false none:true none:false,left:false left:false right:true, |
            | a,g       | start,first,second,third,fourth,fourth | depart,turn left,turn left,turn left,end of road left,arrive  | ,left:true left:true none:false none:false,left:true left:true none:false none:false,left:true left:true none:false none:false,left:true left:true right:false,     |
