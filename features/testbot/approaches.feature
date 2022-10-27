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

    Scenario: Start End opposite approach, option unrestricted for Start and End
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

    Scenario: Start End opposite approach, option unrestricted for Start and curb for End
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
            | from | to | approaches        | route    |
            | s    | e  | unrestricted curb | ab,bc |

    Scenario: Test on oneway segment, Start End opposite approach, option unrestricted for Start and End
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

    Scenario: Test on oneway segment, Start End opposite approach, option unrestricted for Start and curb for End
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
            | from | to | approaches        | route    |
            | s    | e  | unrestricted curb | ab,bc,bc |


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

    Scenario: [Left-hand-side] Start End opposite approach, option unrestricted for Start and End
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

    Scenario: [Left-hand-side] Start End opposite approach, option unrestricted for Start and curb for End
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
        And the node map
            """
               s      e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | approaches        | route |
            | s    | e  | unrestricted curb | ab,bc |