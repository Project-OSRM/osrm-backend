@routing  @guidance @left-handed
Feature: Basic Roundabout

    Background:
        Given a grid size of 10 meters
        Given the profile file "car" initialized with
            """
            profile.properties.left_hand_driving = true
            """

    Scenario: Roundabout exit counting for left sided driving
        Given a grid size of 10 meters
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
        Given a grid size of 10 meters
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
           | waypoints | route           | turns                                                    |
           | c,a       | cba,cba,cba     | depart,exit roundabout left,arrive                       |
           | l,a       | lkj,cba,cba,cba | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | i,a       | ihg,cba,cba,cba | depart,roundabout-exit-3,exit roundabout straight,arrive |
