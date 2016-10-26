@routing @guidance @turn-lanes
Feature: Turn Lane Guidance

    Background:
        Given the profile "car"
        Given a grid size of 3 meters


    @sliproads
    Scenario: Separate Turn Lanes
        Given the node map
            """
                          e
                          .
            a ... b ..... c . g
                    `     .
                     `... d
                          .
                          f
            """

        And the ways
            | nodes | turn:lanes:forward | name     | oneway |
            | ab    |                    | in       | yes    |
            | bc    | left\|through      | in       | yes    |
            | bd    | right              | in       | yes    |
            | ec    |                    | cross    | no     |
            | cd    |                    | cross    | no     |
            | df    |                    | cross    | no     |
            | cg    |                    | straight | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | cd     | d        | no_left_turn  |
            | restriction | bc       | cd     | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                           | lanes                                  |
            | a,e       | in,cross,cross       | depart,turn left,arrive         | ,left:true straight:false right:false, |
            | a,g       | in,straight,straight | depart,new name straight,arrive | ,left:false straight:true right:false, |
            | a,f       | in,cross,cross       | depart,turn right,arrive        | ,left:false straight:false right:true, |

    @sliproads
    Scenario: Separate Turn Lanes
        Given the node map
            """
                          e
            a . . b . . . c g
                    `     .
                       `  .
                        ` d
                          f
            """

        And the ways
            | nodes | turn:lanes:forward | name     | oneway |
            | ab    |                    | in       | yes    |
            | bc    | left\|through      | in       | yes    |
            | bd    | right              | in       | yes    |
            | ec    |                    | cross    | no     |
            | cd    |                    | cross    | no     |
            | df    |                    | cross    | no     |
            | cg    |                    | straight | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | cd     | d        | no_left_turn  |
            | restriction | bc       | cd     | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                           | lanes                                  |
            | a,e       | in,cross,cross       | depart,turn left,arrive         | ,left:true straight:false right:false, |
            | a,g       | in,straight,straight | depart,new name straight,arrive | ,left:false straight:true right:false, |
            | a,f       | in,cross,cross       | depart,turn right,arrive        | ,left:false straight:false right:true, |


    @sliproads
    Scenario: Separate Turn Lanes Next to other turns
        Given the node map
            """
                        . e
            a . . b . . . c g
                  .   `   .
                  .     ` .
                  .       d
                  .       f
                  .
                  .
                  .
                  .
            i . . h . . . j
            """

        And the ways
            | nodes | turn:lanes:forward | name     | oneway |
            | ab    |                    | in       | yes    |
            | bc    | left\|through      | in       | yes    |
            | bd    | right              | in       | yes    |
            | ec    |                    | cross    | no     |
            | cd    |                    | cross    | no     |
            | df    |                    | cross    | no     |
            | cg    |                    | straight | no     |
            | bh    | left\|right        | turn     | yes    |
            | ihj   |                    | other    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | cd     | d        | no_left_turn  |
            | restriction | bc       | cd     | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                               | lanes                                  |
            | a,e       | in,cross,cross       | depart,turn left,arrive             | ,left:true straight:false right:false, |
            | a,g       | in,straight,straight | depart,new name straight,arrive     | ,left:false straight:true right:false, |
            | a,f       | in,cross,cross       | depart,turn right,arrive            | ,left:false straight:false right:true, |
            | a,j       | in,turn,other,other  | depart,turn right,turn left,arrive  | ,,left:true right:false,               |
            | a,i       | in,turn,other,other  | depart,turn right,turn right,arrive | ,,left:false right:true,               |


    @todo @2654 @none
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

    @todo @2650 @sliproads
    #market and haight in SF, restricted turn
    #http://www.openstreetmap.org/#map=19/37.77308/-122.42238
    Scenario: Market/Haight without Through Street
        Given the node map
            """
                          g j





                              f
                            e
                          d
            a           b c






                    l     h i
            """

        And the ways
            | nodes | name   | highway     | oneway | turn:lanes:forward |
            | ab    | ghough | secondary   | yes    |                    |
            | bc    | ghough | secondary   | yes    | through\|through   |
            | bd    | ghough | secondary   | yes    | none\|through      |
            | def   | ghough | secondary   | yes    |                    |
            | gd    | market | primary     | yes    |                    |
            | dc    | market | primary     | yes    |                    |
            | ch    | market | primary     | yes    |                    |
            | iej   | market | primary     | yes    |                    |
            | bl    | haight | residential | yes    | left\|none         |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | relation    | bd       | dc     | d        | no_right_turn |

        When I route I should get
            | waypoints | route                | turns                              | lanes                                                    |
            | a,l       | ghough,haight,haight | depart,turn right,arrive           | ,none:false straight:false straight:false straight:true, |
            | a,h       | ghough,market,market | depart,turn slight right,arrive    | ,none:false straight:false straight:true straight:true,  |
            | a,j       | ghough,market,market | depart,turn left,arrive            | ,none:true straight:false straight:false straight:false, |
            | a,f       | ghough,ghough,ghough | depart,continue slight left,arrive | ,none:true straight:true straight:false straight:false,  |

    @todo @2650 @sliproads
    #market and haight in SF, unrestricted
    #http://www.openstreetmap.org/#map=19/37.77308/-122.42238
    Scenario: Market/Haight without Through Street
        Given the node map
            """
                          g j





                              f
                            e
                          d
            a           b c






                    l     h i
            """

        And the ways
            | nodes | name   | highway     | oneway | turn:lanes:forward |
            | ab    | ghough | secondary   | yes    |                    |
            | bc    | ghough | secondary   | yes    | through\|through   |
            | bd    | ghough | secondary   | yes    | none\|through      |
            | def   | ghough | secondary   | yes    |                    |
            | gd    | market | primary     | yes    |                    |
            | dc    | market | primary     | yes    |                    |
            | ch    | market | primary     | yes    |                    |
            | iej   | market | primary     | yes    |                    |
            | bl    | haight | residential | yes    | left\|none         |

        When I route I should get
            | waypoints | route                | turns                              | lanes                                                    |
            | a,l       | ghough,haight,haight | depart,turn right,arrive           | ,none:false straight:false straight:false straight:true, |
            | a,h       | ghough,market,market | depart,turn slight right,arrive    | ,none:false straight:false straight:true straight:true,  |
            | a,j       | ghough,market,market | depart,turn left,arrive            | ,none:true straight:false straight:false straight:false, |
            | a,f       | ghough,ghough,ghough | depart,continue slight left,arrive | ,none:true straight:true straight:false straight:false,  |


    Scenario: Check sliproad handler loop's exit condition, Issue #2896
      # http://www.openstreetmap.org/way/198481519
        Given the node locations
            | node | lat        | lon         |
            | a    | 7.6125350  | 126.5708309 |
            | b    | 7.6125156  | 126.5707219 |
            | c    | 7.6125363  | 126.5708337 |

        And the ways
            | nodes | name |
            | cbac  |      |

        When I route I should get
            | from | to | route | turns         |
            | a    | c  | ,     | depart,arrive |
