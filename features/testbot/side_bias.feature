@routing @testbot @sidebias
Feature: Testbot - side bias

    Background:
        Given the lua profile file
        """
        require 'testbot'
        properties.left_hand_driving = true
        """

    Scenario: Left hand bias
        Given the profile file "testbot.lua" extended with
        """
        properties.left_hand_driving = true
        function turn_function (angle)
          local k = 10 * angle * angle * 50 / (90.0 * 90.0)
          return (angle >= 0) and k * 1.2 or k / 1.2
        end
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
            | d    | a  | bd,ab,ab | 82s +-1    |
            | d    | c  | bd,bc,bc | 100s +-1   |

    Scenario: Right hand bias
        Given the profile file "testbot.lua" extended with
        """
        properties.left_hand_driving = false
        function turn_function (angle)
          local k = 10 * angle * angle * 50 / (90.0 * 90.0)
          return (angle >= 0) and k / 1.2 or k * 1.2
        end
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
            | d    | a  | bd,ab,ab | 100s +-1   |
            | d    | c  | bd,bc,bc | 82s +-1    |

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
