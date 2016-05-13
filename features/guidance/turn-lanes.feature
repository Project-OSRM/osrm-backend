@routing @guidance @turn-lanes
Feature: Turn Lane Guidance

    Background:
        Given the profile "car"
        Given a grid size of 20 meters

    #requires https://github.com/cucumber/cucumber-js/issues/417
    #Due to this, we use & as a pipe character. Switch them out for \| when 417 is fixed
    @bug @WORKAROUND-FIXME
    Scenario: Basic Turn Lane 3-way Turn with empty lanes
        Given the node map
            | a |   | b |   | c |
            |   |   | d |   |   |

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward | turn:lanes:backward | name     |
            | ab     |                | through\|right     |                     | in       |
            | bc     |                |                    | left\|through&&     | straight |
            | bd     |                |                    | left\|right         | right    |

       When I route I should get
            | waypoints | route                | turns                           | lanes   |
            | a,c       | in,straight,straight | depart,new name straight,arrive | ,1,     |
            | a,d       | in,right,right       | depart,turn right,arrive        | ,0,     |
            | c,a       | straight,in,in       | depart,new name straight,arrive | ,0 1 2, |
            | c,d       | straight,right,right | depart,turn left,arrive         | ,3,     |

    Scenario: Basic Turn Lane 4-Way Turn
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   | d |   |   |

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward | turn:lanes:backward | name     |
            | ab     |                | \|right            |                     | in       |
            | bc     |                |                    |                     | straight |
            | bd     |                |                    | left\|              | right    |
            | be     |                |                    |                     | left     |

       When I route I should get
            | waypoints | route                   | turns                           | lanes |
            | a,c       | in,straight,straight    | depart,new name straight,arrive | ,1,   |
            | a,d       | in,right,right          | depart,turn right,arrive        | ,0,   |
            | a,e       | in,left,left            | depart,turn left,arrive         | ,1,   |
            | d,a       | right,in,in             | depart,turn left,arrive         | ,1,   |
            | d,e       | right,left,left         | depart,new name straight,arrive | ,0,   |
            | d,c       | right,straight,straight | depart,turn right,arrive        | ,0,   |

    Scenario: Basic Turn Lane 4-Way Turn using none
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   | d |   |   |

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward | turn:lanes:backward | name     |
            | ab     |                | none\|right        |                     | in       |
            | bc     |                |                    |                     | straight |
            | bd     |                |                    | left\|none          | right    |
            | be     |                |                    |                     | left     |

       When I route I should get
            | waypoints | route                | turns                           | lanes |
            | a,c       | in,straight,straight | depart,new name straight,arrive | ,1,   |
            | a,d       | in,right,right       | depart,turn right,arrive        | ,0,   |
            | a,e       | in,left,left         | depart,turn left,arrive         | ,1,   |

    Scenario: Basic Turn Lane 4-Way With U-Turn Lane
        Given the node map
            |   |   | e |   |   |
            | a | 1 | b |   | c |
            |   |   | d |   |   |

        And the ways
            | nodes  | turn:lanes     | turn:lanes:forward          | name     |
            | ab     |                | reverse;left\|through;right | in       |
            | bc     |                |                             | straight |
            | bd     |                |                             | right    |
            | be     |                |                             | left     |

       When I route I should get
            | from | to | bearings        | route                | turns                           | lanes |
            | a    | c  | 180,180 180,180 | in,straight,straight | depart,new name straight,arrive | ,0,   |
            | a    | d  | 180,180 180,180 | in,right,right       | depart,turn right,arrive        | ,0,   |
            | a    | e  | 180,180 180,180 | in,left,left         | depart,turn left,arrive         | ,1,   |
            | 1    | a  | 90,2 270,2      | in,in,in             | depart,turn uturn,arrive        | ,1,   |


    #this next test requires decision on how to announce lanes for going straight if there is no turn
    @TODO @WORKAROUND-FIXME
    Scenario: Turn with Bus-Lane
        Given the node map
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   | d |   |   |

        And the ways
            | nodes | name | turn:lanes:forward | lanes:psv:forward |
            | ab    | road | through\|right&    | 1                 |
            | bc    | road |                    |                   |
            | bd    | turn |                    |                   |

        When I route I should get
            | waypoints | route          | turns                           | lanes |
            | a,d       | road,turn,turn | depart,turn right,arrive        | ,0,   |
            | a,c       | road,road,road | depart,use lane straight,arrive | ,1,   |

    #turn lanes are often drawn at the incoming road, even though the actual turn requires crossing the intersection first
    @TODO @WORKAROUND-FIXME
    Scenario: Turn Lanes at Segregated Road
        Given the node map
            |   |   | i | l |   |   |
            |   |   |   |   |   |   |
            | h |   | g | f |   | e |
            | a |   | b | c |   | d |
            |   |   |   |   |   |   |
            |   |   | j | k |   |   |

        And the ways
            | nodes | name  | turn:lanes:forward      | oneway |
            | ab    | road  | left\|through&right     | yes    |
            | bc    | road  | left\|through           | yes    |
            | cd    | road  |                         | yes    |
            | ef    | road  | \|through&through;right | yes    |
            | fg    | road  | left;through\|through&  | yes    |
            | gh    | road  |                         | yes    |
            | ig    | cross |                         | yes    |
            | gb    | cross | left\|through           | yes    |
            | bj    | cross |                         | yes    |
            | kc    | cross | left\|through;right     | yes    |
            | cf    | cross | left\|through           | yes    |
            | fl    | cross |                         | yes    |

        When I route I should get
            | waypoints | route             | turns                           | lanes | #                          |
            | a,j       | road,cross,cross  | depart,turn right,arrive        | ,0,   |                            |
            | a,d       | road,road,road    | depart,use lane straight,arrive | ,1,   | #post-processing reduction |
            | a,l       | road,cross,cross  | depart,turn left,arrive         | ,2,   |                            |
            | a,h       | road,road,road    | depart,continue uturn,arrive    | ,2,   |                            |
            | k,d       | cross,road,road   | depart,turn right,arrive        | ,0,   |                            |
            | k,l       | cross,cross,cross | depart,use lane straight,arrive | ,0,   |                            |
            | k,h       | cross,road,road   | depart,turn left,arrive         | ,1,   |                            |
            | k,j       | cross,cross,cross | depart,continue uturn,arrive    | ,1,   |                            |
            | e,l       | road,cross,cross  | depart,turn right,arrive        | ,0,   |                            |
            | e,h       | road,road         | depart,arrive                   | ,     |                            |
            | e,j       | road,cross,cross  | depart,turn left,arrive         | ,2,   |                            |
            | e,d       | road,road,road    | depart,continue uturn,arrive    | ,2,   |                            |
            | i,h       | cross,road,road   | depart,turn right,arrive        | ,,    |                            |
            | i,j       | cross,cross,cross | depart,use lane straight,arrive | ,0,   |                            |
            | i,d       | cross,road,road   | depart,turn left,arrive         | ,1,   |                            |
            | i,l       | cross,cross,cross | depart,continue uturn,arrive    | ,1,   |                            |

    Scenario: Turn Lanes at Segregated Road
        Given the node map
            |   |   | g | f |   |   |
            | a |   | b | c |   | d |
            |   |   |   |   |   |   |
            |   |   | j | k |   |   |

        And the ways
            | nodes | name  | turn:lanes:forward      | oneway |
            | ab    | road  | left\|through&right     | yes    |
            | bc    | road  |                         | yes    |
            | cd    | road  |                         | yes    |
            | gb    | cross |                         | yes    |
            | bj    | cross |                         | yes    |
            | kc    | cross |                         | yes    |
            | cf    | cross |                         | yes    |

        When I route I should get
            | waypoints | route             | turns                           | lanes |
            | a,j       | road,cross,cross  | depart,turn right,arrive        | ,0,   |

    #this can happen due to traffic lights / lanes not drawn up to the intersection itself
    Scenario: Turn Lanes Given earlier than actual turn
        Given the node map
            | a |   | b | c |   | d |
            |   |   |   |   |   |   |
            |   |   |   | e |   |   |

        And the ways
            | nodes | name | turn:lanes:forward |
            | ab    | road | \|right            |
            | bc    | road |                    |
            | cd    | road |                    |
            | ce    | turn |                    |

        When I route I should get
            | waypoints | route          | turns                           | lanes |
            | a,e       | road,turn,turn | depart,turn right,arrive        | ,0,   |
            | a,d       | road,road,road | depart,use lane straight,arrive | ,1,   |

    Scenario: Turn Lanes Given earlier than actual turn
        Given the node map
            | a |   | b | c | d |   | e |   | f | g | h |   | i |
            |   |   | j |   |   |   |   |   |   |   | k |   |   |

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
            | waypoints | route                        | turns                           | lanes |
            | a,k       | road,second-turn,second-turn | depart,turn right,arrive        | ,0,   |
            | a,i       | road,road,road               | depart,use lane straight,arrive | ,1,   |
            | i,j       | road,first-turn,first-turn   | depart,turn left,arrive         | ,1,   |
            | i,a       | road,road,road               | depart,use lane straight,arrive | ,0,   |

    Scenario: Passing a one-way street
        Given the node map
            | e |   |   | f |   |
            | a |   | b | c | d |

        And the ways
            | nodes | name | turn:lanes:forward | oneway |
            | ab    | road | left\|through      | no     |
            | bcd   | road |                    | no     |
            | eb    | owi  |                    | yes    |
            | cf    | turn |                    |        |

        When I route I should get
            | waypoints | route          | turns                   | lanes |
            | a,f       | road,turn,turn | depart,turn left,arrive | ,1,   |

    Scenario: Passing a one-way street, partly pulled back lanes
        Given the node map
            | e |   |   | f |   |
            | a |   | b | c | d |
            |   |   | g |   |   |

        And the ways
            | nodes | name  | turn:lanes:forward  | oneway |
            | ab    | road  | left\|through;right | no     |
            | bcd   | road  |                     | no     |
            | eb    | owi   |                     | yes    |
            | cf    | turn  |                     | no     |
            | bg    | right |                     | no     |

        When I route I should get
            | waypoints | route            | turns                    | lanes |
            | a,f       | road,turn,turn   | depart,turn left,arrive  | ,1,   |
            | a,g       | road,right,right | depart,turn right,arrive | ,0,   |

    Scenario: Passing a one-way street, partly pulled back lanes, no through
        Given the node map
            | e |   |   | f |
            | a |   | b | c |
            |   |   | g |   |

        And the ways
            | nodes | name  | turn:lanes:forward  | oneway |
            | ab    | road  | left\|right         | no     |
            | bc    | road  |                     | no     |
            | eb    | owi   |                     | yes    |
            | cf    | turn  |                     | no     |
            | bg    | right |                     | no     |

        When I route I should get
            | waypoints | route            | turns                    | lanes |
            | a,f       | road,turn,turn   | depart,turn left,arrive  | ,1,   |
            | a,g       | road,right,right | depart,turn right,arrive | ,0,   |

     Scenario: Narrowing Turn Lanes
        Given the node map
            |   |   |   |   | g |   |
            |   |   |   |   |   |   |
            | a |   | b | c | d | e |
            |   |   |   | f |   |   |

        And the ways
            | nodes | name    | turn:lanes:forward  |
            | ab    | road    | left\|through&right |
            | bc    | road    |                     |
            | cd    | road    | left\|through       |
            | de    | through |                     |
            | dg    | left    |                     |
            | cf    | right   |                     |

        When I route I should get
            | waypoints | route                | turns                           | lanes |
            | a,g       | road,left,left       | depart,turn left,arrive         | ,2,   |
            | a,e       | road,through,through | depart,new name straight,arrive | ,1,   |
            | a,f       | road,right,right     | depart,turn right,arrive        | ,0,   |

    Scenario: Anticipate Lane Change
        Given the node map
            | a |   | b |   | x |
            |   |   |   |   |   |
            |   |   | c |   | d |
            |   |   |   |   |   |
            |   |   | y |   |   |

        And the ways
            | nodes | turn:lanes:forward   | turn:lanes:backward |
            | ab    | through\|right&right |                     |
            | bx    |                      | left\|left&through  |
            | bc    | left\|through        | left\|right         |
            | cd    |                      | left\|right         |
            | cy    |                      |                     |

       When I route I should get
            | waypoints | route       | turns                                            | lanes |
            | d,a       | cd,bc,ab,ab | depart,end of road right,end of road left,arrive | ,0,1, |

     Scenario: Turn at a traffic light
        Given the node map
            | a | b | c | d |
            |   |   | e |   |

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
            | waypoints | route          | turns                           | lanes |
            | a,d       | road,road,road | depart,use lane straight,arrive | ,1,   |
            | a,e       | road,turn,turn | depart,turn right,arrive        | ,0,   |


     Scenario: Theodor Heuss Platz
        Given the node map
            |   |   |   | i | o |   |   | l |   |
            |   |   | b |   |   |   | a |   | m |
            |   | c |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   | h |   |
            |   |   |   |   |   |   |   |   |   |
            | j |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   | g |   |
            |   |   |   |   |   |   |   |   |   |
            |   | d |   |   |   |   |   |   |   |
            |   |   | e |   |   |   | f |   |   |
            |   |   |   |   | k |   |   |   | n |

        And the nodes
            | node | highway         |
            | g    | traffic_signals |

        And the ways
            | nodes         | name          | turn:lanes:forward                                              | junction   | oneway | highway   |
            | abcdef        | roundabout    |                                                                 | roundabout | yes    | primary   |
            | gha           | roundabout    |                                                                 | roundabout | yes    | primary   |
            | fg            | roundabout    | slight_left\|slight_left;slight_right&slight_right&slight_right | roundabout | yes    | primary   |
            | aoib          | top           |                                                                 |            | yes    | primary   |
            | cjd           | left          |                                                                 |            | yes    | primary   |
            | ekf           | bottom        |                                                                 |            | yes    | primary   |
            | fng           | bottom-right  |                                                                 |            | yes    | primary   |
            | hma           | top-right     |                                                                 |            | yes    | primary   |
            | hl            | top-right-out |                                                                 |            | yes    | secondary |

        When I route I should get
            | waypoints | route                           | turns                           | lanes   |
            | i,m       | top,top-right,top-right         | depart,roundabout-exit-4,arrive | ,0 1 2, |
            | i,l       | top,top-right-out,top-right-out | depart,roundabout-exit-4,arrive | ,2 3,   |
            | i,o       | top,top,top                     | depart,roundabout-exit-5,arrive | ,,      |

    Scenario: Turn Lanes Breaking up
        Given the node map
            |   |   |   | g |   |
            |   |   |   |   |   |
            |   |   |   | c |   |
            | a | b |   | d | e |
            |   |   |   |   |   |
            |   |   |   | f |   |

        And the ways
            | nodes | name  | turn:lanes:forward         | oneway | highway   |
            | ab    | road  | left\|left&through&through | yes    | primary   |
            | bd    | road  | through\|through           | yes    | primary   |
            | bc    | road  | left\|left                 | yes    | primary   |
            | de    | road  |                            | yes    | primary   |
            | fdcg  | cross |                            |        | secondary |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | fdcg   | d        | no_left_turn  |
            | restriction | bc       | fdcg   | c        | no_right_turn |

        When I route I should get
            | waypoints | route            | turns                           | lanes |
            | a,g       | road,cross,cross | depart,turn left,arrive         | ,2 3, |
            | a,e       | road,road,road   | depart,use lane straight,arrive | ,0 1, |

    Scenario: U-Turn Road at Intersection
        Given the node map
            |   |   |   |   |   | h |   |
            |   |   |   |   | f | e | j |
            | a | b |   |   |   |   |   |
            |   |   |   |   | c | d | i |
            |   |   |   |   |   | g |   |

        And the ways
            | nodes | name  | turn:lanes:forward | oneway | highway  |
            | ab    | road  |                    | no     | primary  |
            | di    | road  |                    | yes    | primary  |
            | bc    | road  | \|through&right    | yes    | primary  |
            | cd    | road  | \|through&right    | yes    | primary  |
            | fc    | road  |                    | no     | tertiary |
            | jefb  | road  |                    | yes    | primary  |
            | gdeh  | cross |                    | no     | primary  |

       When I route I should get
            | from | to | bearings        | route            | turns                           | lanes |
            | a    | g  | 180,180 180,180 | road,cross,cross | depart,turn right,arrive        | ,0,   |
            | a    | h  | 180,180 180,180 | road,cross,cross | depart,turn left,arrive         | ,2,   |
            | a    | i  | 180,180 180,180 | road,road,road   | depart,use lane straight,arrive | ,1 2, |
            | b    | a  | 90,2 270,2      | road,road,road   | depart,continue uturn,arrive    | ,2,   |

    Scenario: Segregated Intersection Merges With Lanes
        Given the node map
            |   |   |   |   |   |   | f |
            |   |   |   |   |   |   |   |
            | e |   |   | d |   |   |   |
            |   |   |   |   |   | c | g |
            | a |   |   | b |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   | h |   |

        And the ways
            | nodes | name     | turn:lanes:forward              | oneway | highway   |
            | abc   | road     | left\|left&left&through&through | yes    | primary   |
            | cde   | road     |                                 | yes    | primary   |
            | hc    | cross    |                                 | yes    | secondary |
            | cg    | straight |                                 | no     | tertiary  |
            | cf    | left     |                                 | yes    | primary   |

        When I route I should get
            | waypoints | route                  | turns                           | lanes   |
            | a,f       | road,left,left         | depart,turn left,arrive         | ,2 3 4, |
            | a,e       | road,road,road         | depart,turn uturn,arrive        | ,4,     |
            | a,g       | road,straight,straight | depart,new name straight,arrive | ,0 1,   |

     Scenario: Passing Through a Roundabout
        Given the node map
            |   |   | h |   | g |   |   |
            |   | a |   |   |   | f | k |
            | i |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   | b |   |   |   | e |   |
            |   |   | c |   | d |   |   |
            |   |   |   |   | j |   |   |

        And the ways
            | nodes | name   | turn:lanes:forward                    | oneway | highway   | junction   |
            | efgha | round  |                                       | yes    | primary   | roundabout |
            | ab    | round  |                                       | yes    | primary   | roundabout |
            | bc    | round  | slight_left\|slight_left&slight_right | yes    | primary   | roundabout |
            | cd    | round  |                                       | yes    | primary   | roundabout |
            | de    | round  | slight_left\|slight_right             | yes    | primary   | roundabout |
            | ib    | left   | slight_left\|slight_left&slight_right | yes    | primary   |            |
            | cj    | bottom |                                       | yes    | primary   |            |
            | ek    | right  |                                       | yes    | primary   |            |

        When I route I should get
            | waypoints | route              | turns                      | lanes |
            | i,j       | left,bottom,bottom | depart,round-exit-1,arrive | ,0,   |
            | i,k       | left,right,right   | depart,round-exit-2,arrive | ,1,   |

    Scenario: Crossing Traffic Light
        Given the node map
            | a |   | b |   | c |   | d |
            |   |   |   |   |   |   | e |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        And the ways
            | nodes | name  | turn:lanes:forward                                 | highway |
            | abc   | road  | through\|through&through;slight_right&slight_right | primary |
            | cd    | road  |                                                    | primary |
            | ce    | cross |                                                    | primary |

        When I route I should get
            | waypoints | route            | turns                           | lanes   |
            | a,d       | road,road,road   | depart,use lane straight,arrive | ,1 2 3, |
            | a,e       | road,cross,cross | depart,turn slight right,arrive | ,0 1,   |

    Scenario: Highway Ramp
        Given the node map
            | a |   | b |   | c |   | d |
            |   |   |   |   |   |   | e |

        And the ways
            | nodes | name | turn:lanes:forward                                 | highway       |
            | abc   | hwy  | through\|through&through;slight_right&slight_right | motorway      |
            | cd    | hwy  |                                                    | motorway      |
            | ce    | ramp |                                                    | motorway_link |

        When I route I should get
            | waypoints | route         | turns                               | lanes   |
            | a,d       | hwy,hwy,hwy   | depart,use lane straight,arrive     | ,1 2 3, |
            | a,e       | hwy,ramp,ramp | depart,off ramp slight right,arrive | ,0 1,   |

     Scenario: Turning Off Ramp
        Given the node map
            |   | a |   |
            | d | c | b |
            | e | f | g |
            |   | h |   |

        And the ways
            | nodes | name | turn:lanes:forward | highway       | oneway |
            | ac    | off  | left\|right        | motorway_link | yes    |
            | bcd   | road |                    | primary       | yes    |
            | cf    | road |                    | primary       |        |
            | efg   | road |                    | primary       | yes    |
            | fh    | on   |                    | motorway_link | yes    |

        When I route I should get
            | waypoints | route         | turns                    | lanes |
            | a,d       | off,road,road | depart,turn_right,arrive | ,0,   |
            | a,g       | off,road,road | depart,turn_left,arrive  | ,1,   |
            | a,h       |               |                          |       |

     Scenario: Off Ramp In a Turn
        Given the node map
            | a |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   | b |   |   |   |   |   | c |
            |   |   |   |   |   |   |   |   |   |   | d |   |

        And the ways
            | nodes | name | turn:lanes:forward            | highway       | oneway |
            | ab    | hwy  | through\|through&slight_right | motorway      | yes    |
            | bc    | hwy  |                               | motorway      | yes    |
            | bd    | ramp |                               | motorway_link | yes    |

        When I route I should get
            | waypoints | route         | turns                               | lanes |
            | a,c       | hwy,hwy,hwy   | depart,use lane slight left,arrive  | ,1 2, |
            | a,d       | hwy,ramp,ramp | depart,off ramp slight right,arrive | ,0,   |

     Scenario: Reverse Lane in Segregated Road
        Given the node map
            | h |   |   |   |   | g |   |   |   |   |   | f |
            |   |   |   |   |   |   |   | e |   |   |   |   |
            |   |   |   |   |   |   |   | d |   |   |   |   |
            | a |   |   |   |   | b |   |   |   |   |   | c |

        And the ways
            | nodes | name | turn:lanes:forward       | highway      | oneway |
            | ab    | road | reverse\|through&through | primary      | yes    |
            | bc    | road |                          | primary      | yes    |
            | bdeg  | road |                          | primary_link | yes    |
            | fgh   | road |                          | primary      | yes    |

        When I route I should get
            | waypoints | route          | turns                        | lanes |
            | a,h       | road,road,road | depart,continue uturn,arrive | ,2,   |

    Scenario: Reverse Lane in Segregated Road with none
        Given the node map
            | h |   |   |   |   | g |   |   |   |   |   | f |
            |   |   |   |   |   |   |   | e |   |   |   |   |
            |   |   |   |   |   |   |   | d |   |   |   |   |
            | a |   |   |   |   | b |   |   |   |   |   | c |

        And the ways
            | nodes | name | turn:lanes:forward    | highway      | oneway |
            | ab    | road | reverse\|through&none | primary      | yes    |
            | bc    | road |                       | primary      | yes    |
            | bdeg  | road |                       | primary_link | yes    |
            | fgh   | road |                       | primary      | yes    |

        When I route I should get
            | waypoints | route          | turns                        | lanes |
            | a,h       | road,road,road | depart,continue uturn,arrive | ,2,   |

    Scenario: Reverse Lane in Segregated Road with none, Service Turn Prior
        Given the node map
            | h |   |   |   |   | g |   |   |   |   |   | f |
            |   |   |   |   |   |   |   | e |   |   |   |   |
            |   |   |   |   |   |   |   | d |   |   |   |   |
            | a |   | j |   |   | b |   |   |   |   |   | c |
            |   |   | i |   |   |   |   |   |   |   |   |   |

        And the ways
            | nodes | name | turn:lanes:forward    | highway      | oneway |
            | ajb   | road | reverse\|through&none | primary      | yes    |
            | bc    | road |                       | primary      | yes    |
            | bdeg  | road |                       | primary_link | yes    |
            | fgh   | road |                       | primary      | yes    |
            | ji    | park |                       | service      | no     |

        When I route I should get
            | waypoints | route          | turns                        | lanes |
            | a,h       | road,road,road | depart,continue uturn,arrive | ,2,   |

    Scenario: Don't collapse everything to u-turn / too wide
        Given the node map
            | a |   | b |   | e |
            |   |   |   |   |   |
            | d |   | c |   | f |

        And the ways
            | nodes | highway   | name   | turn:lanes:forward |
            | ab    | primary   | road   | through\|right     |
            | bc    | primary   | road   |                    |
            | dc    | primary   | road   | left\|through      |
            | be    | secondary | top    |                    |
            | cf    | secondary | bottom |                    |

        When I route I should get
            | waypoints | turns                                          | route               | lanes |
            | a,d       | depart,continue right,end of road right,arrive | road,road,road,road | ,0,,  |
            | d,a       | depart,continue left,end of road left,arrive   | road,road,road,road | ,1,,  |

     Scenario: Merge Lanes Onto Freeway
        Given the node map
            | a |   |   | b | c |
            |   | d |   |   |   |

        And the ways
            | nodes | highway       | name | turn:lanes:forward         |
            | abc   | motorway      | Hwy  |                            |
            | db    | motorway_link | ramp | slight_right\|slight_right |

        When I route I should get
            | waypoints | turns                           | route        | lanes |
            | d,c       | depart,merge slight left,arrive | ramp,Hwy,Hwy | ,0 1, |

     Scenario: Fork on motorway links - don't fork on through but use lane
        Given the node map
            | i |   |   |   |   | a |
            | j |   | c | b |   | x |

        And the ways
            | nodes | name | highway       | turn:lanes:forward |
            | xb    | xbcj | motorway_link |                    |
            | bc    | xbcj | motorway_link | none\|slight_right |
            | cj    | xbcj | motorway_link |                    |
            | ci    | off  | motorway_link |                    |
            | ab    | on   | motorway_link |                    |

        When I route I should get
            | waypoints | route             | turns                                             | lanes |
            | a,j       | on,xbcj,xbcj,xbcj | depart,merge slight left,use lane straight,arrive | ,,1,  |
            | a,i       | on,xbcj,off,off   | depart,merge slight left,turn slight right,arrive | ,,0,  |
