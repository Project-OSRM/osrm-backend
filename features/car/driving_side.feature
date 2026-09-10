@routing @car @driving_side
Feature: Car - driving side resolved from the way's coordinates

    # No profile property, no driving_side tag, no --location-dependent-data.
    # The answer comes from where the ways are.

    Background:
        Given the profile "car"

    Scenario: Left-hand traffic settled from the extract's extent
        Given the origin -0.1276,51.5072
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
            | from | to | route    | driving_side   |
            | d    | a  | bd,ab,ab | left,left,left |
            | d    | c  | bd,bc,bc | left,left,left |

    Scenario: Right-hand traffic settled from the extract's extent
        Given the origin 2.3522,48.8566
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
            | from | to | route    | driving_side      |
            | d    | a  | bd,ab,ab | right,right,right |
            | d    | c  | bd,bc,bc | right,right,right |

    Scenario: The oncoming penalty follows the resolved side
        Given the origin -0.1276,51.5072
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

        # Traffic drives on the left here, so bd,bc turns across it and bd,ab
        # turns away from it. In right-hand traffic the costs are the other way
        # round, which is what side_bias.feature checks at null island.
        When I route I should get
            | from | to | route    | time      | driving_side   |
            | d    | a  | bd,ab,ab | 24s +-1   | left,left,left |
            | d    | c  | bd,bc,bc | 29.5s +-1 | left,left,left |

    Scenario: An extract spanning both sides is resolved per way
        # The Hong Kong boundary runs along the Shenzhen River, left-hand
        # traffic to the south of it. The polygon edge crosses lon 114.02 at
        # lat 22.5066, dropping about 60m of latitude per 400m of easting, so
        # the rows are spaced wide enough that the tilt cannot reach them.
        Given the origin 114.02,22.5066
        And a grid size of 500 meters
        And the node map
            """
            a   b


            c   d
            """
        And the ways
            | nodes | name  |
            | ab    | north |
            | cd    | south |

        When I route I should get
            | from | to | route              | driving_side      |
            | a    | b  | north,north        | right,right       |
            | c    | d  | south,south        | left,left         |

    Scenario: A way that straddles the boundary is settled on its last node
        Given the origin 114.02,22.5066
        And a grid size of 500 meters
        And the node map
            """
            a


            b
            """
        And the ways
            | nodes | name     |
            | ab    | crossing |

        # a is north of the boundary and b is south of it, so the line query
        # cannot answer. The tie-break is the way's last node, the same node
        # get_location_tag uses to place a way.
        When I route I should get
            | from | to | route             | driving_side |
            | a    | b  | crossing,crossing | left,left    |

    Scenario: The index can be switched off
        Given the origin -0.1276,51.5072
        And the extract extra arguments "--driving-side-index=off"
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
            | from | to | route    | driving_side      |
            | d    | a  | bd,ab,ab | right,right,right |
            | d    | c  | bd,bc,bc | right,right,right |

    Scenario: A driving_side tag outranks the index
        Given the origin -0.1276,51.5072
        And the node map
            """
            a   b   c

                d
            """
        And the ways
            | nodes | driving_side |
            | ab    | right        |
            | bc    | right        |
            | bd    | right        |

        When I route I should get
            | from | to | route    | driving_side      |
            | d    | a  | bd,ab,ab | right,right,right |
            | d    | c  | bd,bc,bc | right,right,right |

    Scenario: An explicit profile setting outranks the index
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = false
        """
        And the origin -0.1276,51.5072
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
            | from | to | route    | driving_side      |
            | d    | a  | bd,ab,ab | right,right,right |
            | d    | c  | bd,bc,bc | right,right,right |
