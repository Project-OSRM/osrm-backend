@routing @testbot @sidebias
Feature: Testbot - side bias

    Background:
        Given the profile file
        """
        require 'testbot'
        properties.left_hand_driving = true
        """

    Scenario: Left hand bias
        Given the profile file "car" extended with
        """
        properties.left_hand_driving = true
        turn_bias = properties.left_hand_driving and 1/1.075 or 1.075
        """
        Given the node map
            """
            a   b   c

                d
            """
        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bd    |

        When I route I should get
            | from | to | route    | time       |
            | d    | a  | bd,ab,ab | 29s +-1    |
            | d    | c  | bd,bc,bc | 33s +-1   |

    Scenario: Right hand bias
        Given the profile file "car" extended with
        """
        properties.left_hand_driving = false
        turn_bias = properties.left_hand_driving and 1/1.075 or 1.075
        """
        And the node map
            """
            a   b   c

                d
            """
        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bd    |

        When I route I should get
            | from | to | route    | time       |
            | d    | a  | bd,ab,ab | 33s +-1   |
            | d    | c  | bd,bc,bc | 29s +-1    |

    Scenario: Roundabout exit counting for left sided driving
        And a grid size of 10 meters
        And the node map
            """
                a
                b
            h g   c d
                e
                f
            """
        And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

        When I route I should get
           | waypoints | route    | turns                                         |
           | a,d       | ab,cd,cd | depart,roundabout turn left exit-1,arrive     |
           | a,f       | ab,ef,ef | depart,roundabout turn straight exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout turn right exit-3,arrive    |

    Scenario: Mixed Entry and Exit
        And a grid size of 10 meters
        And the node map
           """
             c   a
           j   b   f
             k   e
           l   h   d
             g   i
           """

        And the ways
           | nodes | junction   | oneway |
           | cba   |            | yes    |
           | fed   |            | yes    |
           | ihg   |            | yes    |
           | lkj   |            | yes    |
           | behkb | roundabout | yes    |

        When I route I should get
           | waypoints | route       | turns                           |
           | c,a       | cba,cba,cba | depart,roundabout-exit-1,arrive |
           | l,a       | lkj,cba,cba | depart,roundabout-exit-2,arrive |
           | i,a       | ihg,cba,cba | depart,roundabout-exit-3,arrive |
