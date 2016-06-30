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
            | waypoints | route                | turns                       | lanes                                  |
            | a,e       | in,cross,cross       | depart,turn left,arrive     | ,left:true straight:false right:false, |
            | a,g       | in,straight,straight | depart,turn straight,arrive | ,left:false straight:true right:false, |
            | a,f       | in,cross,cross       | depart,turn right,arrive    | ,left:false straight:false right:true, |
