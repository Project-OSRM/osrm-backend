@routing @guidance @turn-lanes
Feature: Turn Lane Guidance

    Background:
        Given the profile "car"
        Given a grid size of 20 meters

    @simple
    Scenario: Basic Turn Lane 3-way Turn with empty lanes
        Given the node map
            """
            a   b   c
                d
            """

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward | turn:lanes:backward | name     |
            | ab     |                | through\|right     |                     | in       |
            | bc     |                |                    | left\|through\|\|   | straight |
            | bd     |                |                    | left\|right         | right    |

       When I route I should get
            | waypoints | route                | turns                           | lanes                                            |
            | a,c       | in,straight,straight | depart,new name straight,arrive | ,straight:true right:false,                      |
            | a,d       | in,right,right       | depart,turn right,arrive        | ,straight:false right:true,                      |
            | c,a       | straight,in,in       | depart,new name straight,arrive | ,left:false straight:true none:true none:true,   |
            | c,d       | straight,right,right | depart,turn left,arrive         | ,left:true straight:false none:false none:false, |

    Scenario: Basic Turn Lane 3-way Turn with designated lane
        Given the node map
            """
            a   b   c
                d
            """

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward      | name     | vehicle:lanes:forward |
            | ab     |                | through\|through\|right | in       | yes\|no\|yes          |
            | bc     |                |                         | straight |                       |
            | bd     |                |                         | right    |                       |

       When I route I should get
            | waypoints | route                | turns                           | lanes                       |
            | a,c       | in,straight,straight | depart,new name straight,arrive | ,straight:true right:false, |
            | a,d       | in,right,right       | depart,turn right,arrive        | ,straight:false right:true, |

    @simple
    Scenario: Basic Turn Lane 4-Way Turn
        Given the node map
            """
                e
            a   b   c
                d
            """

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward | turn:lanes:backward | name     |
            | ab     |                | \|right            |                     | in       |
            | bc     |                |                    |                     | straight |
            | bd     |                |                    | left\|              | right    |
            | be     |                |                    |                     | left     |

       When I route I should get
            | waypoints | route                   | turns                           | lanes                   |
            | a,c       | in,straight,straight    | depart,new name straight,arrive | ,none:true right:false, |
            | a,d       | in,right,right          | depart,turn right,arrive        | ,none:false right:true, |
            | a,e       | in,left,left            | depart,turn left,arrive         | ,none:true right:false, |
            | d,a       | right,in,in             | depart,turn left,arrive         | ,left:true none:false,  |
            | d,e       | right,left,left         | depart,new name straight,arrive | ,left:false none:true,  |
            | d,c       | right,straight,straight | depart,turn right,arrive        | ,left:false none:true,  |

    @simple @none
    Scenario: Basic Turn Lane 4-Way Turn using none
        Given the node map
            """
                e
            a   b   c
                d
            """

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward | turn:lanes:backward | name     |
            | ab     |                | none\|right        |                     | in       |
            | bc     |                |                    |                     | straight |
            | bd     |                |                    | left\|none          | right    |
            | be     |                |                    |                     | left     |

       When I route I should get
            | waypoints | route                | turns                           | lanes                   |
            | a,c       | in,straight,straight | depart,new name straight,arrive | ,none:true right:false, |
            | a,d       | in,right,right       | depart,turn right,arrive        | ,none:false right:true, |
            | a,e       | in,left,left         | depart,turn left,arrive         | ,none:true right:false, |

    @simple @reverse
    Scenario: Basic Turn Lane 4-Way With U-Turn Lane
        Given the node map
            """
                e
            a 1 b   c
                d
            """

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward          | name     |
            | ab     |                | reverse;left\|through;right | in       |
            | bc     |                |                             | straight |
            | bd     |                |                             | right    |
            | be     |                |                             | left     |

       When I route I should get
            | from | to | bearings        | route                | turns                           | lanes                                  |
            | a    | c  | 180,180 180,180 | in,straight,straight | depart,new name straight,arrive | ,left;uturn:false straight;right:true, |
            | a    | d  | 180,180 180,180 | in,right,right       | depart,turn right,arrive        | ,left;uturn:false straight;right:true, |
            | a    | e  | 180,180 180,180 | in,left,left         | depart,turn left,arrive         | ,left;uturn:true straight;right:false, |
            | 1    | a  | 90,2 270,2      | in,in,in             | depart,turn uturn,arrive        | ,left;uturn:true straight;right:false, |


    #this next test requires decision on how to announce lanes for going straight if there is no turn
    @simple @psv @none
    Scenario: Turn with Bus-Lane
        Given the node map
            """
            a   b   c

                d
            """

        And the ways
            | nodes | name | turn:lanes:forward | lanes:psv:forward |
            | ab    | road | through\|right\|   | 1                 |
            | bc    | road |                    |                   |
            | bd    | turn |                    |                   |

        When I route I should get
            | waypoints | route          | turns                    | lanes                       |
            | a,d       | road,turn,turn | depart,turn right,arrive | ,straight:false right:true, |
            | a,c       | road,road      | depart,arrive            | ,                           |

    Scenario: Turn with Bus-Lane Left
        Given the node map
            """
                d

            a   b   c   f
                    e
            """

        And the ways
            | nodes | name | turn:lanes:forward  | lanes:psv:forward | oneway |
            | ab    | road | left\|through\|     | 1                 | yes    |
            | bc    | road |                     |                   | yes    |
            | bd    | turn |                     |                   | yes    |
            | cf    | turn |                     |                   | yes    |
            | ce    | side |                     |                   | yes    |

        When I route I should get
            | waypoints | route          | turns                   | lanes                       |
            | a,d       | road,turn,turn | depart,turn left,arrive | ,left:true straight:false,  |
            | a,c       | road,road      | depart,arrive           | ,                           |

    # This tests whether empty/invalid PSV tags cause osrm-extract to crash
    Scenario: Turn with Bus-Lane
        Given the node map
            """
            a   b   c

                d
            """

        And the ways
            | nodes | name | turn:lanes:forward | lanes:psv:forward | lanes:psv:backward |
            | ab    | road | through\|right\|   | 1                 | foo                |
            | bc    | road |                    |                   |                    |
            | bd    | turn |                    |                   |                    |

        When I route I should get
            | waypoints | route          | turns                    | lanes                       |
            | a,d       | road,turn,turn | depart,turn right,arrive | ,straight:false right:true, |

    @simple @psv
    Scenario: Turn with Bus-Lane but without lanes
        Given the node map
            """
            a   b   c

                d
            """

        And the ways
            | nodes | name | lanes:psv |
            | ab    | road | 1         |
            | bc    | road | yes       |
            | bd    | turn |           |

        When I route I should get
            | waypoints | route          | turns                    |
            | a,d       | road,turn,turn | depart,turn right,arrive |
            | a,c       | road,road      | depart,arrive            |

    #turn lanes are often drawn at the incoming road, even though the actual turn requires crossing the intersection first
    @todo @collapse @partition-lanes
    Scenario: Turn Lanes at Segregated Road
        Given the node map
            """
                i l

            h   g f   e
            a   b c   d

                j k
            """

        And the ways
            | nodes | name  | turn:lanes:forward       | oneway |
            | ab    | road  | left\|through\|right     | yes    |
            | bc    | road  | left\|through            | yes    |
            | cd    | road  |                          | yes    |
            | ef    | road  | \|through\|through;right | yes    |
            | fg    | road  | left;through\|through\|  | yes    |
            | gh    | road  |                          | yes    |
            | ig    | cross |                          | yes    |
            | gb    | cross | left\|through            | yes    |
            | bj    | cross |                          | yes    |
            | kc    | cross | left\|through;right      | yes    |
            | cf    | cross | left\|through            | yes    |
            | fl    | cross |                          | yes    |

        When I route I should get
            | waypoints | route             | turns                           | lanes                                           |
            | a,j       | road,cross,cross  | depart,turn right,arrive        | ,left:false straight:false right:true           |
            | a,d       | road,road,road    | depart,use lane straight,arrive | ,left:false straight:true right:false,          |
            | a,l       | road,cross,cross  | depart,turn left,arrive         | ,left:true straight:false right:false,          |
            | a,h       | road,road,road    | depart,continue uturn,arrive    | ,left:true straight:false right:false,          |
            | k,d       | cross,road,road   | depart,turn right,arrive        | ,left:false straight;right:true,                |
            | k,l       | cross,cross,cross | depart,use lane straight,arrive | ,left:false straight;right:true,                |
            | k,h       | cross,road,road   | depart,turn left,arrive         | ,left:true straight;right:false,                |
            | k,j       | cross,cross,cross | depart,continue uturn,arrive    | ,left:true straight;right:false,                |
            | e,l       | road,cross,cross  | depart,turn right,arrive        | ,none:false straight:false straight;right:true, |
            | e,h       | road,road         | depart,arrive                   | ,none:false straight:true straight;right:true   |
            | e,j       | road,cross,cross  | depart,turn left,arrive         | ,none:true straight:false straight;right:false, |
            | e,d       | road,road,road    | depart,continue uturn,arrive    | ,none:true straight:false straight;right:false, |
            | i,h       | cross,road,road   | depart,turn right,arrive        | ,,                                              |
            | i,j       | cross,cross,cross | depart,use lane straight,arrive | ,left:false straight:true,                      |
            | i,d       | cross,road,road   | depart,turn left,arrive         | ,left:true straight:false,                      |
            | i,l       | cross,cross,cross | depart,continue uturn,arrive    | ,left:true straight:false,                      |

    #copy of former case to prevent further regression
    @collapse @partition-lanes
    Scenario: Turn Lanes at Segregated Road
        Given the node map
            """
                i l

            h   g f   e
            a   b c   d

                j k
            """

        And the ways
            | nodes | name  | turn:lanes:forward       | oneway |
            | ab    | road  | left\|through\|right     | yes    |
            | bc    | road  | left\|through            | yes    |
            | cd    | road  |                          | yes    |
            | ef    | road  | \|through\|through;right | yes    |
            | fg    | road  | left;through\|through\|  | yes    |
            | gh    | road  |                          | yes    |
            | ig    | cross |                          | yes    |
            | gb    | cross | left\|through            | yes    |
            | bj    | cross |                          | yes    |
            | kc    | cross | left\|through;right      | yes    |
            | cf    | cross | left\|through            | yes    |
            | fl    | cross |                          | yes    |

        When I route I should get
            | waypoints | route             | turns                        | lanes                                           |
            | a,j       | road,cross,cross  | depart,turn right,arrive     | ,left:false straight:false right:true,          |
            | k,d       | cross,road,road   | depart,turn right,arrive     | ,left:false straight;right:true,                |
            | e,l       | road,cross,cross  | depart,turn right,arrive     | ,none:false straight:false straight;right:true, |
            | i,h       | cross,road,road   | depart,turn right,arrive     | ,,                                              |
            | i,j       | cross,cross       | depart,arrive                | ,                                               |
            | i,l       | cross,cross,cross | depart,continue uturn,arrive | ,left:true straight:false,                      |

    @partition-lanes
    Scenario: Turn Lanes at Segregated Road
        Given the node map
            """
                g f
            a   b c   d

                j k
            """

        And the ways
            | nodes | name  | turn:lanes:forward   | oneway |
            | ab    | road  | left\|through\|right | yes    |
            | bc    | road  |                      | yes    |
            | cd    | road  |                      | yes    |
            | gb    | cross |                      | yes    |
            | bj    | cross |                      | yes    |
            | kc    | cross |                      | yes    |
            | cf    | cross |                      | yes    |

        When I route I should get
            | waypoints | route             | turns                           | lanes                                  |
            | a,j       | road,cross,cross  | depart,turn right,arrive        | ,left:false straight:false right:true, |

    #this can happen due to traffic lights / lanes not drawn up to the intersection itself
    @2654 @previous-lanes
    Scenario: Turn Lanes Given earlier than actual turn
        Given the node map
            """
            a   b c   d

                  e
            """

        And the ways
            | nodes | name | turn:lanes:forward |
            | ab    | road | \|right            |
            | bc    | road |                    |
            | cd    | road |                    |
            | ce    | turn |                    |

        When I route I should get
            | waypoints | route          | turns                    | lanes                   |
            | a,e       | road,turn,turn | depart,turn right,arrive | ,none:false right:true, |
            | a,d       | road,road      | depart,arrive            | ,                       |

    @2654 @previous-lanes
    Scenario: Turn Lanes Given earlier than actual turn
        Given the node map
            """
            a   b c d   e   f g h   i
                j               k
            """

        And the ways
            | nodes | name        | turn:lanes:forward | turn:lanes:backward |
            | abc   | road        |                    |                     |
            | cd    | road        |                    | left\|              |
            | def   | road        |                    |                     |
            | fg    | road        | \|right            |                     |
            | ghi   | road        |                    |                     |
            | bj    | first-turn  |                    |                     |
            | hk    | second-turn |                    |                     |

        When I route I should get
            | waypoints | route                        | turns                    | lanes                   |
            | a,k       | road,second-turn,second-turn | depart,turn right,arrive | ,none:false right:true, |
            | a,i       | road,road                    | depart,arrive            | ,                       |
            | i,j       | road,first-turn,first-turn   | depart,turn left,arrive  | ,left:true none:false,  |
            | i,a       | road,road                    | depart,arrive            | ,                       |

    @previous-lanes
    Scenario: Passing a one-way street
        Given the node map
            """
            e     f
            a   b c d
            """

        And the ways
            | nodes | name | turn:lanes:forward | oneway |
            | ab    | road | left\|through      | no     |
            | bcd   | road |                    | no     |
            | eb    | owi  |                    | yes    |
            | cf    | turn |                    |        |

        When I route I should get
            | waypoints | route          | turns                   | lanes                      |
            | a,f       | road,turn,turn | depart,turn left,arrive | ,left:true straight:false, |

    @partition-lanes
    Scenario: Passing a one-way street, partly pulled back lanes
        Given the node map
            """
            e     f
            a   b c d
                g
            """

        And the ways
            | nodes | name  | turn:lanes:forward  | oneway |
            | ab    | road  | left\|through;right | no     |
            | bcd   | road  |                     | no     |
            | eb    | owi   |                     | yes    |
            | cf    | turn  |                     | no     |
            | bg    | right |                     | no     |

        When I route I should get
            | waypoints | route            | turns                    | lanes                            |
            | a,f       | road,turn,turn   | depart,turn left,arrive  | ,left:true straight;right:false, |
            | a,d       | road,road        | depart,arrive            | ,                                |
            | a,g       | road,right,right | depart,turn right,arrive | ,left:false straight;right:true, |

    @partition-lanes @previous-lanes
    Scenario: Passing a one-way street, partly pulled back lanes, no through
        Given the node map
            """
            e     f
            a   b c
                g
            """

        And the ways
            | nodes | name  | turn:lanes:forward  | oneway |
            | ab    | road  | left\|right         | no     |
            | bc    | road  |                     | no     |
            | eb    | owi   |                     | yes    |
            | cf    | turn  |                     | no     |
            | bg    | right |                     | no     |

        When I route I should get
            | waypoints | route            | turns                    | lanes                   |
            | a,f       | road,turn,turn   | depart,turn left,arrive  | ,left:true right:false, |
            | a,g       | road,right,right | depart,turn right,arrive | ,left:false right:true, |

    @todo @partition-lanes @previous-lanes
    Scenario: Narrowing Turn Lanes
        Given the node map
            """
                    g

            a   b c d e
                  f
            """

        And the ways
            | nodes | name    | turn:lanes:forward   |
            | ab    | road    | left\|through\|right |
            | bc    | road    |                      |
            | cd    | road    | left\|through        |
            | de    | through |                      |
            | dg    | left    |                      |
            | cf    | right   |                      |

        When I route I should get
            | waypoints | route                | turns                           | lanes                                  |
            | a,g       | road,left,left       | depart,turn left,arrive         | ,left:true straight:false right:false, |
            | a,e       | road,through,through | depart,new name straight,arrive | ,left:false straight:true right:false, |
            | a,f       | road,right,right     | depart,turn right,arrive        | ,left:false straight:false right:true, |

    @previous-lanes
    Scenario: Turn at a traffic light
        Given the node map
            """
            a b c d
                e
            """

        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        And the ways
            | nodes | name | turn:lanes:forward |
            | ab    | road | through\|right     |
            | bc    | road |                    |
            | cd    | road |                    |
            | ce    | turn |                    |

        When I route I should get
            | waypoints | route          | turns                    | lanes                       |
            | a,d       | road,road      | depart,arrive            | ,                           |
            | a,e       | road,turn,turn | depart,turn right,arrive | ,straight:false right:true, |

    @todo @roundabout
    Scenario: Theodor Heuss Platz
        Given the node map
            """
                  i o     l
                b       a   m
              c
                          h

            j
                          g

              d
                e       f
                    k       n
            """

        And the nodes
            | node | highway         |
            | g    | traffic_signals |

        And the ways
            | nodes         | name          | turn:lanes:forward                                                | junction   | oneway | highway   |
            | abcdef        | roundabout    |                                                                   | roundabout | yes    | primary   |
            | gha           | roundabout    |                                                                   | roundabout | yes    | primary   |
            | fg            | roundabout    | slight_left\|slight_left;slight_right\|slight_right\|slight_right | roundabout | yes    | primary   |
            | aoib          | top           |                                                                   |            | yes    | primary   |
            | cjd           | left          |                                                                   |            | yes    | primary   |
            | ekf           | bottom        |                                                                   |            | yes    | primary   |
            | fng           | bottom-right  |                                                                   |            | yes    | primary   |
            | hma           | top-right     |                                                                   |            | yes    | primary   |
            | hl            | top-right-out |                                                                   |            | yes    | secondary |

        When I route I should get
            | waypoints | route                           | turns                           | lanes                                                                                  |
            | i,m       | top,top-right,top-right         | depart,roundabout-exit-4,arrive | ,slight left:false slight left;slight right:true slight right:true slight right:true,  |
            | i,l       | top,top-right-out,top-right-out | depart,roundabout-exit-4,arrive | ,slight left:true slight left;slight right:true slight right:false slight right:false, |
            | i,o       | top,top,top                     | depart,roundabout-exit-5,arrive | ,,                                                                                     |

    @sliproads
    Scenario: Turn Lanes Breaking up
        Given the node map
            """
                  g

                  c
            a b   d e

                  f
            """

        And the ways
            | nodes | name  | turn:lanes:forward                  | oneway | highway   |
            | ab    | road  | left\|left\|through\|through\|right | yes    | primary   |
            | bd    | road  | through\|through\|right             | yes    | primary   |
            | bc    | road  | left\|left                          | yes    | primary   |
            | de    | road  |                                     | yes    | primary   |
            | fd    | cross |                                     |        | secondary |
            |  dc   | cross |                                     |        | secondary |
            |   cg  | cross |                                     |        | secondary |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | dc     | d        | no_left_turn  |
            | restriction | bc       | dc     | c        | no_right_turn |

        When I route I should get
            | waypoints | route            | turns                   | lanes                                                           |
            | a,g       | road,cross,cross | depart,turn left,arrive | ,left:true left:true straight:false straight:false right:false, |
            | a,e       | road,road        | depart,arrive           | ,                                                               |

    #NEEDS TO BE INVESTIGATED. Turn restriction shouldn't be here. See #2867
    @reverse @previous-lanes
    Scenario: U-Turn Road at Intersection
        Given the node map
            """
                        h
                    f   e j
            a b
                    c   d i
                        g
            """

        And the ways
            | nodes | name  | turn:lanes:forward | oneway | highway  |
            | ab    | road  |                    | no     | primary  |
            | di    | road  |                    | yes    | primary  |
            | bc    | road  | \|through\|right   | yes    | primary  |
            | cd    | road  | \|through\|right   | yes    | primary  |
            | fc    | road  |                    | no     | tertiary |
            | je    | road  |                    | yes    | primary  |
            | ef    | road  |                    | yes    | primary  |
            | fb    | road  |                    | yes    | primary  |
            | eh    | cross |                    | no     | primary  |
            | de    | cross |                    | no     | primary  |
            | gd    | cross |                    | no     | primary  |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | de       | ef     | e        | no_left_turn  |

        When I route I should get
            | from | to | bearings        | route            | turns                        | lanes                                  |
            | a    | g  | 180,180 180,180 | road,cross,cross | depart,turn right,arrive     | ,none:false straight:false right:true, |
            | a    | h  | 180,180 180,180 | road,cross,cross | depart,turn left,arrive      | ,none:true straight:false right:false, |
            | a    | i  | 180,180 180,180 | road,road        | depart,arrive                | ,                                      |
            | b    | a  | 90,2 270,2      | road,road,road   | depart,continue uturn,arrive | ,none:true straight:false right:false, |

    @reverse
    Scenario: Segregated Intersection Merges With Lanes
        Given the node map
            """
                        f

            e     d
                      c g
            a     b

                      h
            """

        And the ways
            | nodes | name     | turn:lanes:forward                 | oneway | highway   |
            | abc   | road     | left\|left\|left\|through\|through | yes    | primary   |
            | cde   | road     |                                    | yes    | primary   |
            | hc    | cross    |                                    | yes    | secondary |
            | cg    | straight |                                    | no     | tertiary  |
            | cf    | left     |                                    | yes    | primary   |

        When I route I should get
            | waypoints | route                  | turns                           | lanes                                                           |
            | a,f       | road,left,left         | depart,turn left,arrive         | ,left:true left:true left:true straight:false straight:false,   |
            | a,e       | road,road,road         | depart,turn uturn,arrive        | ,left:true left:false left:false straight:false straight:false, |
            | a,g       | road,straight,straight | depart,new name straight,arrive | ,left:false left:false left:false straight:true straight:true,  |

    @todo @roundabout
    Scenario: Passing Through a Roundabout
        Given the node map
            """
                h   g
              a       f k
            i

              b       e
                c   d
                    j
            """

        And the ways
            | nodes | name   | turn:lanes:forward                     | oneway | highway   | junction   |
            | efgha | round  |                                        | yes    | primary   | roundabout |
            | ab    | round  |                                        | yes    | primary   | roundabout |
            | bc    | round  | slight_left\|slight_left\|slight_right | yes    | primary   | roundabout |
            | cd    | round  |                                        | yes    | primary   | roundabout |
            | de    | round  | slight_left\|slight_right              | yes    | primary   | roundabout |
            | ib    | left   | slight_left\|slight_left\|slight_right | yes    | primary   |            |
            | cj    | bottom |                                        | yes    | primary   |            |
            | ek    | right  |                                        | yes    | primary   |            |

        When I route I should get
            | waypoints | route              | turns                      | lanes |
            | i,j       | left,bottom,bottom | depart,round-exit-1,arrive | ,0,   |
            | i,k       | left,right,right   | depart,round-exit-2,arrive | ,1,   |

    @previous-lanes
    Scenario: Crossing Traffic Light
        Given the node map
            """
            a   b   c   d
                        e
            """

        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        And the ways
            | nodes | name  | turn:lanes:forward                                   | highway |
            | abc   | road  | through\|through\|through;slight_right\|slight_right | primary |
            | cd    | road  |                                                      | primary |
            | ce    | cross |                                                      | primary |

        When I route I should get
            | waypoints | route            | turns                           | lanes                                                                        |
            | a,d       | road,road        | depart,arrive                   | ,                                                                            |
            | a,e       | road,cross,cross | depart,turn slight right,arrive | ,straight:false straight:false straight;slight right:true slight right:true, |

    @ramp
    Scenario: Highway Ramp
        Given the node map
            """
            a   b   c   d
                        e
            """

        And the ways
            | nodes | name | turn:lanes:forward                                   | highway       |
            | abc   | hwy  | through\|through\|through;slight_right\|slight_right | motorway      |
            | cd    | hwy  |                                                      | motorway      |
            | ce    | ramp |                                                      | motorway_link |

        When I route I should get
            | waypoints | route         | turns                               | lanes                                                                        |
            | a,d       | hwy,hwy       | depart,arrive                       | ,                                                                            |
            | a,e       | hwy,ramp,ramp | depart,off ramp slight right,arrive | ,straight:false straight:false straight;slight right:true slight right:true, |

    @todo
    Scenario: Turning Off Ramp
        Given the node map
            """
              a
            d c b
            e f g
              h
            """

        And the ways
            | nodes | name | turn:lanes:forward | highway       | oneway |
            | ac    | off  | left\|right        | motorway_link | yes    |
            | bcd   | road |                    | primary       | yes    |
            | cf    | road |                    | primary       |        |
            | efg   | road |                    | primary       | yes    |
            | fh    | on   |                    | motorway_link | yes    |

        When I route I should get
            | waypoints | route         | turns                    | lanes                   |
            | a,d       | off,road,road | depart,turn_right,arrive | ,left:false right:true, |
            | a,g       | off,road,road | depart,turn_left,arrive  | ,left:true right:false, |
            | a,h       |               |                          |                         |

    @ramp
    Scenario: Off Ramp In a Turn
        Given the node map
            """
            a

                      b           c
                                d
            """

        And the ways
            | nodes | name | turn:lanes:forward             | highway       | oneway |
            | ab    | hwy  | through\|through\|slight_right | motorway      | yes    |
            | bc    | hwy  |                                | motorway      | yes    |
            | bd    | ramp |                                | motorway_link | yes    |

        When I route I should get
            | waypoints | route         | turns                               | lanes                                             |
            | a,c       | hwy,hwy       | depart,arrive                       | ,                                                 |
            | a,d       | hwy,ramp,ramp | depart,off ramp slight right,arrive | ,straight:false straight:false slight right:true, |

    @reverse
    Scenario: Reverse Lane in Segregated Road
        Given the node map
            """
            h         g           f
                          e
                          d
            a         b           c
            """

        And the ways
            | nodes | name | turn:lanes:forward        | highway      | oneway |
            | ab    | road | reverse\|through\|through | primary      | yes    |
            | bc    | road |                           | primary      | yes    |
            | bdeg  | road |                           | primary_link | yes    |
            | fgh   | road |                           | primary      | yes    |

        When I route I should get
            | waypoints | route          | turns                        | lanes                                     |
            | a,h       | road,road,road | depart,continue uturn,arrive | ,uturn:true straight:false straight:false,|

    @reverse
    Scenario: Reverse Lane in Segregated Road with none
        Given the node map
            """
            h         g           f
                          e
                          d
            a         b           c
            """

        And the ways
            | nodes | name | turn:lanes:forward     | highway      | oneway |
            | ab    | road | reverse\|through\|none | primary      | yes    |
            | bc    | road |                        | primary      | yes    |
            | bdeg  | road |                        | primary_link | yes    |
            | fgh   | road |                        | primary      | yes    |

        When I route I should get
            | waypoints | route          | turns                        | lanes                                  |
            | a,h       | road,road,road | depart,continue uturn,arrive | ,uturn:true straight:false none:false, |

    @reverse
    Scenario: Reverse Lane in Segregated Road with none, Service Turn Prior
        Given the node map
            """
            h         g           f
                          e
                          d
            a   j     b           c
                i
            """

        And the ways
            | nodes | name | turn:lanes:forward     | highway      | oneway |
            | ajb   | road | reverse\|through\|none | primary      | yes    |
            | bc    | road |                        | primary      | yes    |
            | bdeg  | road |                        | primary_link | yes    |
            | fgh   | road |                        | primary      | yes    |
            | ji    | park |                        | service      | no     |

        When I route I should get
            | waypoints | route          | turns                        | lanes                                  |
            | a,h       | road,road,road | depart,continue uturn,arrive | ,uturn:true straight:false none:false, |

    @simple
    Scenario: Don't collapse everything to u-turn / too wide
        Given the node map
            """
            a   b   e

            d   c   f
            """

        And the ways
            | nodes | highway   | name   | turn:lanes:forward |
            | ab    | primary   | road   | through\|right     |
            | bc    | primary   | road   |                    |
            | dc    | primary   | road   | left\|through      |
            | be    | secondary | top    |                    |
            | cf    | secondary | bottom |                    |

        When I route I should get
            | waypoints | turns                                   | route               | lanes                        |
            | a,d       | depart,continue right,turn right,arrive | road,road,road,road | ,straight:false right:true,, |
            | d,a       | depart,continue left,turn left,arrive   | road,road,road,road | ,left:true straight:false,,  |

    @simple
    Scenario: Merge Lanes Onto Freeway
        Given the node map
            """
            a     b c
              d
            """

        And the ways
            | nodes | highway       | name | turn:lanes:forward         |
            | abc   | motorway      | Hwy  |                            |
            | db    | motorway_link | ramp | slight_right\|slight_right |

        When I route I should get
            | waypoints | turns                           | route        | lanes                                 |
            | d,c       | depart,merge slight left,arrive | ramp,Hwy,Hwy | ,slight right:true slight right:true, |

    @2654 @simple
    Scenario: Fork on motorway links - don't fork on through but use lane
        Given the node map
            """
            i         a
            j   c b   x
            """

        And the ways
            | nodes | name | highway       | turn:lanes:forward |
            | xb    | xbcj | motorway_link |                    |
            | bc    | xbcj | motorway_link | none\|slight_right |
            | cj    | xbcj | motorway_link |                    |
            | ci    | off  | motorway_link |                    |
            | ab    | on   | motorway_link |                    |

        When I route I should get
            | waypoints | route        | turns                           | lanes                          |
            | a,j       | on,xbcj      | depart,arrive                   | ,                              |
            | a,i       | on,off,off   | depart,turn slight right,arrive | ,none:false slight right:true, |

    #http://www.openstreetmap.org/#map=17/52.47414/13.35712
    @todo @ramp @2645
    Scenario: Kreuz Schoeneberg - Continue on ramp, don't merge
        Given the node map
            """
            i         j
                    k
            h g   l     f                   e
            d               c         b     a
            """

        And the ways
            | nodes | highway       | name  | oneway | lanes | turn:lanes            |
            | ab    | motorway      | A 100 | yes    | 3     |                       |
            | bc    | motorway      | A 100 | yes    | 4     | \|\|\|slight_right    |
            | cd    | motorway      | A 100 | yes    | 3     |                       |
            | eb    | motorway_link |       | yes    | 1     |                       |
            | cf    | motorway_link |       | yes    | 1     |                       |
            | fj    | motorway_link |       | yes    | 1     |                       |
            | fl    | motorway_link |       | yes    | 1     |                       |
            | kl    | motorway_link |       | yes    | 1     |                       |
            | lg    | motorway_link |       | yes    | 2     | through\|slight_right |
            | gi    | motorway_link |       | yes    | 1     |                       |
            | gh    | motorway_link |       | yes    | 1     |                       |

        When I route I should get
            | waypoints | route        | turns                                                           | lanes                                                                                  |
            | a,d       | A 100,A 100  | depart,arrive                                                   |                                                                                        |
            | a,i       | A 100,,,,    | depart,off ramp slight right,fork slight left,turn right,arrive | ,none:false none:false none:false slight right:true,,straight:false slight right:true, |
            | e,j       | ,,,          | depart,off ramp slight right,fork slight right,arrive           | ,none:false none:false none:false slight right:true,,,                                 |
            | e,i       | ,,,,         | depart,off ramp right,fork slight left,use lane straight,arrive | ,none:false none:false none:false slight right:true,,straight:false slight right:true, |
            | e,d       | ,A 100,A 100 | depart,merge slight left,arrive                                 | ,,                                                                                     |
            | e,h       | ,,,,         | depart,off ramp right,fork left,use lane straight,arrive        | ,none:false none:false none:false slight right:true,,straight:true slight right:false, |

    @collapse @use-lane
    Scenario: Collapse Multiple Use Lanes
        Given the node map
            """
            x a   b     c     d
                  e     f
            """

        And the ways
            | nodes | name | highway | turn:lanes:forward |
            | ab    | road | primary | through;right      |
            | bc    | road | primary | through;right      |
            | cd    | road | primary |                    |
            | xa    | road | primary |                    |
            | be    | turn | primary |                    |
            | cf    | turn | primary |                    |

        When I route I should get
            | waypoints | route     | turns         | lanes |
            | x,d       | road,road | depart,arrive | ,     |

    @partition-lanes
    Scenario: Partitioned turn, Slight Curve
        Given the node map
            """
                f   e
                |   |
                |   |
                |   c
            a - b ' |
                g   d
            """

        And the ways
            | nodes | name  | highway | oneway | turn:lanes:forward |
            | ab    | road  | primary | yes    | left\|right        |
            | bc    | cross | primary | yes    |                    |
            | fbg   | cross | primary | yes    |                    |
            | dce   | cross | primary | yes    |                    |

        When I route I should get
            | waypoints | route            | turns                    | lanes                   |
            | a,g       | road,cross,cross | depart,turn right,arrive | ,left:false right:true, |
            | a,e       | road,cross,cross | depart,turn left,arrive  | ,left:true right:false, |

    Scenario: Lane Parsing Issue #2694
        Given the node map
            """
              c
            a b
              d
            """

        And the ways
            | nodes | highway | turn:lanes:forward |
            | ab    | primary | left;left\|right   |
            | bc    | primary |                    |
            | bd    | primary |                    |

        When I route I should get
            | waypoints | route     | turns                   | lanes                   |
            | a,c       | ab,bc,bc  | depart,turn left,arrive | ,left:true right:false, |

    # http://www.openstreetmap.org/#map=19/47.97685/7.82933&layers=D
    @todo
    Scenario: Lane Parsing Issue #2706: None Assignments I
        Given the node map
            """
              f     j

            a b c   d e

                    i
              g     h
            """

        And the nodes
            | node | highway         |
            | a    | traffic_signals |
            | i    | traffic_signals |

        And the ways
            | nodes | highway        | name           | oneway | turn:lanes:forward       |
            | ab    | secondary      | Wiesentalstr   |        | through;left\|right      |
            | bc    | secondary      | Wiesentalstr   |        | none\|left;through       |
            | cd    | secondary      | Wiesentalstr   |        | none\|left;through       |
            | de    | residential    | Wippertstr     |        |                          |
            | fb    | secondary      | Merzhauser Str | yes    | through\|through\|right  |
            | bg    | secondary      | Merzhauser Str | yes    |                          |
            | hi    | secondary      | Merzhauser Str | yes    | left;reverse\|none\|none |
            | ic    | secondary_link | Merzhauser Str | yes    |                          |
            | id    | secondary      | Merzhauser Str | yes    | through;right\|none      |
            | dj    | secondary      | Merzhauser Str | yes    |                          |

        And the relations
            | type        | way:from | way:to | node:via | restriction    |
            | restriction | fb       | fb     | b        | no_left_turn   |
            | restriction | ic       | cb     | c        | only_left_turn |
            | restriction | id       | dc     | d        | no_left_turn   |

        When I route I should get
            | waypoints | route     | turns                   | lanes                  |
            | h,a ||||
            # Note: at the moment we don't care about routes, we care about the extract process triggering assertions

    # https://www.openstreetmap.org/#map=19/47.99257/7.83276&layers=D
    @todo
    Scenario: Lane Parsing Issue #2706: None Assignments II
        Given the node map
            """
              k l
            j a b f
            i c d e
              h g
            """

        And the ways
            | nodes | highway        | name           | oneway | turn:lanes                           |
            | ka    | secondary      | Eschholzstr    | yes    | left;reverse\|through\|through\|none |
            | kj    | unclassified   | kj             | yes    |                                      |
            | ac    | secondary      | Eschholzstr    | yes    | left;reverse\|none\|none\|none       |
            | ch    | secondary      | Eschholzstr    | yes    |                                      |
            | gd    | secondary      | Eschholzstr    | yes    | left;reverse\|through\|through\|none |
            | db    | secondary      | Eschholzstr    | yes    | left;reverse\|through\|through\|none |
            | bl    | secondary      | Eschholzstr    | yes    |                                      |
            | fb    | residential    | Haslacher Str  | yes    | left;reverse\|left;through\|right    |
            | ba    | secondary_link | Haslacher Str  | yes    | left;reverse\|left;through           |
            | aj    | unclassified   | Haslacher Str  | yes    |                                      |
            | ic    | unclassified   | Haslacher Str  | yes    | left;reverse\|left\|through          |
            | cd    | secondary_link | Haslacher Str  | yes    | left;reverse\|left\|through          |
            | de    | residential    | Haslacher Str  | yes    |                                      |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | ka       | ac     | a        | only_straight_on |
            | restriction | ic       | cd     | c        | only_straight_on |
            | restriction | gd       | db     | d        | only_straight_on |

        When I route I should get
            | waypoints | route     | turns                   | lanes                  |
            | i,e ||||
            # Note: at the moment we don't care about routes, we care about the extract process triggering assertions

    @todo
    Scenario: Lane Parsing Issue #2706: None Assignments III - Minimal reproduction recipe
        Given the node map
            """
                l
              a b
                d

            """

        And the ways
            | nodes | highway        | name           | oneway | turn:lanes                           |
            | db    | secondary      | Eschholzstr    | yes    | left;reverse\|through\|through\|none |
            | bl    | secondary      | Eschholzstr    | yes    |                                      |
            | ba    | secondary_link | Haslacher Str  | yes    |                                      |

        When I route I should get
            | waypoints | route     | turns                   | lanes                  |
            | d,a ||||
            # Note: at the moment we don't care about routes, we care about the extract process triggering assertions

    @reverse @2730 @todo
    Scenario: Reverse on the right
        Given the node map
            """
            a     c
                  b d
            f     e
            """

        And the ways
            | nodes | highway | name    | turn:lanes:forward           | oneway |
            | ab    | primary | in      | left\|through\|right;reverse | yes    |
            | bc    | primary | left    |                              | no     |
            | bd    | primary | through |                              | no     |
            | be    | primary | right   |                              | no     |
            | bf    | primary | in      |                              | yes    |

        When I route I should get
            | waypoints | route              | turns                           | lanes                                        |
            | a,c       | in,left,left       | depart,turn left,arrive         | ,left:true straight:false right;uturn:false, |
            | a,d       | in,through,through | depart,new name straight,arrive | ,left:false straight:true right;uturn:false, |
            | a,e       | in,right,right     | depart,turn right,arrive        | ,left:false straight:false right;uturn:true, |

    @todo @2654
    #https://github.com/Project-OSRM/osrm-backend/issues/2645
    #http://www.openstreetmap.org/export#map=19/52.56054/13.32152
    Scenario: Kurt-Schuhmacher-Damm
        Given the node map
            """
                  g   f

            j     h   e

            a     b   c
                  i   d
            """

        And the ways
            | nodes | name | highway        | oneway | turn:lanes        |
            | ab    |      | motorway_link  | yes    | left\|none\|right |
            | bc    |      | primary_link   | yes    |                   |
            | cd    | ksd  | secondary      | yes    |                   |
            | cef   | ksd  | primary        | yes    |                   |
            | hj    |      | motorway_link  | yes    |                   |
            | eh    |      | secondary_link | yes    |                   |
            | gh    | ksd  | primary        | yes    |                   |
            | hbi   | ksd  | secondary      | yes    |                   |

        When I route I should get
            | waypoints | route    | turns                    | lanes                             |
            | a,f       | ,ksd,ksd | depart,turn left,arrive  | ,left:true none:true right:false, |
            | a,i       | ,ksd,ksd | depart,turn right,arrive | ,left:false none:true right:true, |

    Scenario: Reverse Not Allowed
        Given the node map
            """
                        n o
            f - - e\- - g-j-m
                    d   | |
            a - 1 b/- - c-i-l
                        h k
            """

        And the ways
            | nodes | name | highway        | oneway | turn:lanes:forward   |
            | abc   | road | secondary      | yes    | left\|through\|right |
            | cil   | road | secondary      | yes    |                      |
            | mjgef | road | secondary      | yes    |                      |
            | bde   | road | secondary_link | yes    |                      |
            | ngch  | turn | secondary      | yes    |                      |
            | kijo  | turn | secondary      | yes    |                      |

        When I route I should get
            | waypoints | bearings     | route | turns |
            | 1,a       | 90,2 180,180 |       |       |
