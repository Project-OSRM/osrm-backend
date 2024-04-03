@routing @approach @testbot
Feature: Approach parameter

    Background:
        Given a grid size of 10 meters

    Scenario: Start End same approach, option unrestricted for Start and End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches                | route |
            | s    | e  | unrestricted unrestricted | ab,bc |

    Scenario: Start End same approach, option unrestricted for Start and curb for End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches        | route    |
            | s    | e  | unrestricted curb | ab,bc,bc |

    Scenario: Start End same approach, option unrestricted for Start and opposite for End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches            | route |
            | s    | e  | unrestricted opposite | ab,bc |

    Scenario: Start End same approach, option opposite for Start and curb for End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches            | route    |
            | s    | e  | opposite curb         | ab,bc,bc |

    Scenario: Start End different approach, option unrestricted for Start and End
        Given the profile "testbot"
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches                | route |
            | s    | e  | unrestricted unrestricted | ab,bc |

    Scenario: Start End different approach, option unrestricted for Start and curb for End
        Given the profile "testbot"
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches        | route |
            | s    | e  | unrestricted curb | ab,bc |

    Scenario: Start End different approach, option unrestricted for Start and opposite for End
        Given the profile "testbot"
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches            | route    |
            | s    | e  | unrestricted opposite | ab,bc,bc |

    Scenario: Start End different approach, option curb for Start and opposite for End
        Given the profile "testbot"
        And the node map
            """
               e
            a------b------c-----------d
                              s
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        When I route I should get
            | from | to | approaches    | route       |
            | s    | e  | curb opposite | cd,cd,ab,ab |


    ###############
    # Oneway Test #
    ###############


    Scenario: Test on oneway segment, Start End same approach, option unrestricted for Start and End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches                | route |
            | s    | e  | unrestricted unrestricted | ab,bc |

    Scenario: Test on oneway segment, Start End same approach, option unrestricted for Start and curb for End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches        | route |
            | s    | e  | unrestricted curb | ab,bc |

    Scenario: Test on oneway segment, Start End same approach, option unrestricted for Start and opposite for End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches            | route |
            | s    | e  | unrestricted opposite | ab,bc |

    Scenario: Test on oneway segment, Start End same approach, option opposite for Start and curb for End
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches    | route |
            | s    | e  | opposite curb | ab,bc |

    Scenario: Test on oneway segment, Start End different approach, option unrestricted for Start and End
        Given the profile "testbot"
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches                | route |
            | s    | e  | unrestricted unrestricted | ab,bc |

    Scenario: Test on oneway segment, Start End different approach, option unrestricted for Start and curb for End
        Given the profile "testbot"
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches        | route |
            | s    | e  | unrestricted curb | ab,bc |

    Scenario: Test on oneway segment, Start End different approach, option unrestricted for Start and opposite for End
        Given the profile "testbot"
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches            | route |
            | s    | e  | unrestricted opposite | ab,bc |

    Scenario: Test on oneway segment, Start End different approach, option curb for Start and opposite for End
        Given the profile "testbot"
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | approaches    | route |
            | s    | e  | curb opposite | ab,bc |

    ##############
    # UTurn Test #
    ##############

    Scenario: UTurn test, router can't found a route because uturn unauthorized on the segment selected
        Given the profile "testbot"
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        And the relations
            | type        | way:from | way:to | node:via | restriction |
            | restriction | bc       | bc     | c        | no_u_turn   |

        When I route I should get
            | from | to | approaches        | route |
            | s    | e  | unrestricted curb |       |

    Scenario: UTurn test, router can find a route because uturn authorized to reach opposite side
        Given the profile "testbot"
        And the node map
            """
               e        s
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        And the relations
            | type        | way:from | way:to | node:via | restriction |
            | restriction | bc       | bc     | c        | no_u_turn   |

        When I route I should get
            | from | to | approaches    | route     |
            | s    | e  | curb opposite | bc,ab,ab  |


    Scenario: UTurn test, router can find a route because he can use the roundabout
        Given the profile "testbot"
        And the node map
            """
                             h
               s        e  /   \
            a------b------c     g
                           \   /
                             f
            """

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bc    |            |
            | cfghc | roundabout |

        And the relations
            | type        | way:from | way:to | node:via | restriction |
            | restriction | bc       | bc     | c        | no_u_turn   |

        When I route I should get
            | from | to | approaches         | route    |
            | s    | e  | unrestricted curb  | ab,bc,bc |
            | s    | e  | opposite curb      | ab,bc,bc |


    Scenario: Start End same approach, option unrestricted for Start and curb for End, left-hand driving
        Given the profile file
        """
        local functions = require('testbot')
        local testbot_process_way = functions.process_way
        functions.process_way = function(profile, way, result)
          testbot_process_way(profile, way, result)
          result.is_left_hand_driving = true
        end
        return functions
        """
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches        | route |
            | s    | e  | unrestricted curb | ab,bc |

    Scenario: Start End same approach, option unrestricted for Start and opposite for End, left-hand driving
        Given the profile file
        """
        local functions = require('testbot')
        local testbot_process_way = functions.process_way
        functions.process_way = function(profile, way, result)
          testbot_process_way(profile, way, result)
          result.is_left_hand_driving = true
        end
        return functions
        """
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches            | route    |
            | s    | e  | unrestricted opposite | ab,bc,bc |


    #######################
    # Left-side countries #
    #######################

    Scenario: [Left-hand-side] Start End same approach, option unrestricted for Start and End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches                | route |
            | s    | e  | unrestricted unrestricted | ab,bc |

    Scenario: [Left-hand-side] Start End same approach, option unrestricted for Start and curb for End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s       e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches        | route |
            | s    | e  | unrestricted curb | ab,bc |

    Scenario: [Left-hand-side] Start End same approach, option unrestricted for Start and opposite for End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s       e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches             | route    |
            | s    | e  | unrestricted opposite  | ab,bc,bc |

    Scenario: [Left-hand-side] Start End same approach, option opposite for Start and curb for End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               e      s
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches     | route    |
            | s    | e  | opposite curb  | bc,ab,ab |

    Scenario: [Left-hand-side] Start End different approach, option unrestricted for Start and End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches                | route |
            | s    | e  | unrestricted unrestricted | ab,bc |

    Scenario: [Left-hand-side] Start End different approach, option unrestricted for Start and curb for End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s
            a------b------c
                      e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches        | route    |
            | s    | e  | unrestricted curb | ab,bc,bc |

    Scenario: [Left-hand-side] Start End different approach, option unrestricted for Start and opposite for End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s
            a------b------c
                      e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches            | route |
            | s    | e  | unrestricted opposite | ab,bc |

    Scenario: [Left-hand-side] Start End different approach, option curb for Start and opposite for End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s
            a------b------c
                      e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches       | route |
            | s    | e  | curb opposite    | ab,bc |



    Scenario: Routes with more than two waypoints - uturns allowed
        Given the profile "testbot"
        And the node map
            """
               2                 1
            a------b------c-----------d
                   |
               3   |             4
            e------f------g-----------h
            |
            |
            i

            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | bf    |
            | ef    |
            | fg    |
            | gh    |
            | ei    |

        And the query options
            | continue_straight | false |

        When I route I should get
            | waypoints | approaches                                  | locations             | #                                                   |
            | 1,2,3,4   | curb curb curb curb                         | _,_,_,a,b,f,_,_,i,h,_ | 1,2,2,a,b,f,3,3,i,h,4 (Only u-turn at end of roads) |
            | 1,2,3,4   | curb unrestricted unrestricted curb         | _,_,_,b,f,_,_,h,_     | 1,2,2,b,f,3,3,h,4     (Can u-turn at 2 and 3)       |
            | 1,2,3,4   | opposite opposite opposite opposite         | _,d,a,_,_,b,f,i,_,_,_ | 1,d,a,2,2,b,f,i,3,3,4 (Only u-turn at end of roads) |
            | 1,2,3,4   | opposite unrestricted unrestricted opposite | _,d,_,_,b,f,_,_,_     | 1,d,2,2,b,f,3,3,4     (Can u-turn at 2 and 3)       |


    Scenario: Routes with more than two waypoints - uturns forbidden
        Given the profile "testbot"
        And the node map
            """
               2                 1
            a------b------c-----------d
                   |
               3   |             4
            e------f------g-----------h
            |
            |
            i

            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | bf    |
            | ef    |
            | fg    |
            | gh    |
            | ei    |

        And the query options
            | continue_straight | true |

        When I route I should get
            | waypoints | approaches                                  |  locations            | #                                                   |
            | 1,2,3,4   | curb curb curb curb                         | _,_,_,a,b,f,_,_,i,h,_ | 1,2,2,a,b,f,3,3,i,h,4 (Only u-turn at end of roads) |
            | 1,2,3,4   | curb opposite opposite curb                 | _,a,_,_,b,f,i,_,_,h,_ | 1,a,2,2,b,f,i,3,3,h,4 (switches stops with u-turns) |
            | 1,2,3,4   | opposite opposite opposite opposite         | _,d,a,_,_,b,f,i,_,_,_ | 1,d,a,2,2,b,f,i,3,3,4 (Only u-turn at end of roads) |
            | 1,2,3,4   | opposite curb curb opposite                 | _,d,_,_,a,b,f,_,_,i,_ | 1,d,2,2,a,b,f,3,3,i,4 (switches stops with u-turns) |
