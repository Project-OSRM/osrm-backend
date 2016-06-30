@routing @guidance @turn-lanes
Feature: Turn Lane Guidance

    Background:
        Given the profile "car"
        Given a grid size of 3 meters

    #requires https://github.com/cucumber/cucumber-js/issues/417
    #Due to this, we use & as a pipe character. Switch them out for \| when 417 is fixed
    @bug @WORKAROUND-FIXME
    Scenario: Separate Turn Lanes
        Given the node map
            |   |   |   |   |   |   |   | e |   |
            | a |   |   | b |   |   |   | c | g |
            |   |   |   |   |   |   |   | d |   |
            |   |   |   |   |   |   |   | f |   |

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

    @TODO @2650 @bug
    Scenario: Sliproad with through lane
        Given the node map
            |   |   |   |   |   |   |   |   |   | f |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   | g |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   | e |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   | d |   |   |   |
            | a |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   | b |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   | c |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   | h |   |   |   |   | i |   |   |   |

        And the ways
            | nodes | name   | oneway | turn:lanes:forward |
            | ab    | ghough | yes    |                    |
            | bc    | ghough | yes    | through\|none      |
            | bd    | ghough | yes    | none\|through      |
            | de    | ghough | yes    |                    |
            | fgb   | haight | yes    |                    |
            | bh    | haight | yes    | left\|none         |
            | fd    | market | yes    |                    |
            | dc    | market | yes    |                    |
            | ci    | market | yes    |                    |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | fgb      | bd     | b        | no_left_turn  |
            | restriction | fgb      | bc     | b        | no_left_turn  |

        When I route I should get
            | waypoints | route                | turns                              | lanes |
            | a,h       | ghough,haight,haight | depart,turn right,arrive           |       |
            | a,i       | ghough,market,market | depart,turn right,arrive           |       |
            | a,e       | ghough,ghough,ghough | depart,continue slight left,arrive |       |

    @TODO @2650 @bug
    Scenario: Sliproad with through lane
        Given the node map
            |   |   |   |   |   |   |   | f |   |   |
            |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | g |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   | e |
            |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   | d |   |   |
            | a |   |   |   |   |   |   |   |   |   |
            |   |   |   |   | b |   |   |   |   |   |
            |   |   |   |   |   |   |   | c |   |   |
            |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |
            |   |   |   | h |   |   |   | i |   |   |

        And the ways
            | nodes | name   | oneway | turn:lanes:forward |
            | ab    | ghough | yes    |                    |
            | bc    | ghough | yes    | through\|none      |
            | bd    | ghough | yes    | none\|through      |
            | fgb   | haight | yes    |                    |
            | bh    | haight | yes    | left\|none         |
            | fd    | market | yes    |                    |
            | dc    | market | yes    |                    |
            | ci    | market | yes    |                    |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | dc     | d        | no_right_turn |
            | restriction | fgb      | bd     | b        | no_left_turn  |
            | restriction | fgb      | bc     | b        | no_left_turn  |

        When I route I should get
            | waypoints | route                | turns                              | lanes |
            | a,h       | ghough,haight,haight | depart,turn right,arrive           |       |
            | a,i       | ghough,market,market | depart,turn right,arrive           |       |
            | a,e       | ghough,ghough,ghough | depart,continue slight left,arrive |       |

    Scenario: Separate Turn Lanes
        Given the node map
            |   |   |   |   |   |   |   | e |   |
            | a |   |   | b |   |   |   | c | g |
            |   |   |   |   |   |   |   | d |   |
            |   |   |   |   |   |   |   | f |   |

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

    @guidance @lanes @sliproads
    Scenario: Separate Turn Lanes Next to other turns
        Given the node map
            |   |   |   |   |   |   |   | e |   |
            | a |   |   | b |   |   |   | c | g |
            |   |   |   |   |   |   |   | d |   |
            |   |   |   |   |   |   |   | f |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            | i |   |   | h |   |   |   | j |   |

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

