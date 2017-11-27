@routing @testbot @sidebias
Feature: Testbot - side bias

    Scenario: Left-hand bias
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        profile.turn_bias = 1.075
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
            | from | to | route    | time       | driving_side   |
            | d    | a  | bd,ab,ab | 24s +-1    | left,left,left |
            | d    | c  | bd,bc,bc | 27s +-1    | left,left,left |

    Scenario: Right-hand bias
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        profile.turn_bias = 1 / 1.075
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
            | from | to | route    | time    | driving_side   | #                                   |
            | d    | a  | bd,ab,ab | 27s +-1 | left,left,left | should be inverse of left hand bias |
            | d    | c  | bd,bc,bc | 24s +-1 | left,left,left |                                     |

    Scenario: Roundabout exit counting for left sided driving
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """
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
           | waypoints | route    | driving_side   | turns                                         |
           | a,d       | ab,cd,cd | left,left,left | depart,roundabout turn left exit-1,arrive     |
           | a,f       | ab,ef,ef | left,left,left | depart,roundabout turn straight exit-2,arrive |
           | a,h       | ab,gh,gh | left,left,left | depart,roundabout turn right exit-3,arrive    |


    Scenario: Left-hand bias via location-dependent tags
        Given the profile "car"
        And the node map
            """
            a   b   c

                d
            """
        And the ways with locations
            | nodes |
            | ab    |
            | bc    |
            | bd    |
        And the extract extra arguments "--location-dependent-data test/data/regions/null-island.geojson"

        When I route I should get
            | from | to | route    | driving_side   | time       |
            | d    | a  | bd,ab,ab | left,left,left | 24s +-1    |
            | d    | c  | bd,bc,bc | left,left,left | 27s +-1    |


    Scenario: Left-hand bias via OSM tags
        Given the profile "car"
        And the node map
            """
            a   b   c

                d
            """
        And the ways with locations
            | nodes | driving_side |
            | ab    | right        |
            | bc    | right        |
            | bd    | right        |
        And the extract extra arguments "--location-dependent-data test/data/regions/null-island.geojson"

        When I route I should get
            | from | to | route    | driving_side      | time       |
            | d    | a  | bd,ab,ab | right,right,right | 27s +-1    |
            | d    | c  | bd,bc,bc | right,right,right | 24s +-1    |

    Scenario: changing sides
        Given the profile "car"

        # Note - the boundary in null-island.geojson is at lon = 2.0,
        # and we use the "last node of the way" as the heuristic to detect
        # whether the way is in our out of the driving_side polygon
        And the node locations
            | node | lat | lon  |
            | a    | 0   | 0.5  |
            | b    | 0   | 1.5  |
            | c    | 0   | 2.5  |
            | d    | 0   | 3.5  |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        And the extract extra arguments "--location-dependent-data test/data/regions/null-island.geojson"
        When I route I should get
            | from | to | route       | driving_side           |
            | d    | a  | cd,bc,ab,ab | right,right,left,left  |
            | a    | d  | ab,bc,cd,cd | left,right,right,right |
