@routing @car @relations
Feature: Car - route relations
    Background:
        Given the profile "car"

    Scenario: Assignment using relation membership roles
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
              c----------------d
            """

        And the ways
            | nodes | name        | highway  | ref         |
            | ba    | westbound   | motorway | I 80        |
            | cd    | eastbound   | motorway | I 80;CO 93  |

        And the relations
            | type        | way:east | way:west | route | ref | network |
            | route       | cd       | ba       | road  | 80  | US:I    |
            | route       | cd       | ba       | road  | 93  | US:CO   |


        When I route I should get
            | waypoints | route                | ref                                              |
            | b,a       | westbound,westbound  | I 80 $west,I 80 $west                            |
            | c,d       | eastbound,eastbound  | I 80 $east; CO 93 $east,I 80 $east; CO 93 $east  |

    Scenario: No cardinal directions by default
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = false
        """
        Given the node map
            """
              a----------------b
              c----------------d
            """

        And the ways
            | nodes | name        | highway  | ref         |
            | ba    | westbound   | motorway | I 80        |
            | cd    | eastbound   | motorway | I 80;CO 93  |

        And the relations
            | type        | way:east | way:west | route | ref | network |
            | route       | cd       | ba       | road  | 80  | US:I    |
            | route       | cd       | ba       | road  | 93  | US:CO   |


        When I route I should get
            | waypoints | route                | ref                     |
            | b,a       | westbound,westbound  | I 80,I 80               |
            | c,d       | eastbound,eastbound  | I 80; CO 93,I 80; CO 93 |

    Scenario: No cardinal directions by default
        Given the node map
            """
              a----------------b
              c----------------d
            """

        And the ways
            | nodes | name        | highway  | ref         |
            | ba    | westbound   | motorway | I 80        |
            | cd    | eastbound   | motorway | I 80;CO 93  |

        And the relations
            | type        | way:east | way:west | route | ref | network |
            | route       | cd       | ba       | road  | 80  | US:I    |
            | route       | cd       | ba       | road  | 93  | US:CO   |


        When I route I should get
            | waypoints | route                | ref                     |
            | b,a       | westbound,westbound  | I 80,I 80               |
            | c,d       | eastbound,eastbound  | I 80; CO 93,I 80; CO 93 |


    Scenario: Assignment using relation direction property (no role on members)
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
              c----------------d
            """

        And the ways
            | nodes | name        | highway  | ref         |
            | ba    | westbound   | motorway | I 80        |
            | cd    | eastbound   | motorway | I 80;CO 93  |

        And the relations
            | type        | direction | way | route | ref | network |
            | route       | west      | ba  | road  | 80  | US:I    |
            | route       | east      | cd  | road  | 80  | US:I    |
            | route       | east      | cd  | road  | 93  | US:CO   |

        When I route I should get
            | waypoints | route                | ref                                             |
            | b,a       | westbound,westbound  | I 80 $west,I 80 $west                           |
            | c,d       | eastbound,eastbound  | I 80 $east; CO 93 $east,I 80 $east; CO 93 $east |


    Scenario: Forward assignment on one-way roads using relation direction property
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
              c----------------d
            """

        And the ways
            | nodes | name        | highway  | ref         |
            | ba    | westbound   | motorway | I 80        |
            | cd    | eastbound   | motorway | I 80;CO 93  |

        And the relations
            | type        | direction | way:forward | route | ref | network |
            | route       | west      | ba          | road  | 80  | US:I    |
            | route       | east      | cd          | road  | 80  | US:I    |
            | route       | east      | cd          | road  | 93  | US:CO   |

        When I route I should get
            | waypoints | route                | ref                                             |
            | b,a       | westbound,westbound  | I 80 $west,I 80 $west                           |
            | c,d       | eastbound,eastbound  | I 80 $east; CO 93 $east,I 80 $east; CO 93 $east |


    Scenario: Forward/backward assignment on non-divided roads with role direction tag
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
            """

        And the ways
            | nodes | name      | highway  | ref   | oneway |
            | ab    | mainroad  | motorway | I 80  | no     |

        And the relations
            | type        | direction | way:forward | route | ref | network |
            | route       | west      | ab          | road  | 80  | US:I    |

        And the relations
            | type        | direction | way:backward | route | ref | network |
            | route       | east      | ab           | road  | 80  | US:I    |

        When I route I should get
            | waypoints | route              | ref                    |
            | a,b       | mainroad,mainroad  | I 80 $west,I 80 $west  |
            | b,a       | mainroad,mainroad  | I 80 $east,I 80 $east  |


    Scenario: Conflict between role and direction
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
            """

        And the ways
            | nodes | name       | highway  | ref   |
            | ab    | eastbound  | motorway | I 80  |

        And the relations
            | type        | direction | way:east | route | ref | network |
            | route       | west      | ab       | road  | 80  | US:I    |

        When I route I should get
            | waypoints | route                | ref       |
            | a,b       | eastbound,eastbound  | I 80,I 80 |


    Scenario: Conflict between role and superrelation direction
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
            """

        And the ways
            | nodes | name       | highway  | ref   |
            | ab    | eastbound  | motorway | I 80  |

        And the relations
            | type        | way:east | route | ref | network | name         |
            | route       | ab       | road  | 80  | US:I    | baserelation |

        And the relations
            | type        | direction | relation     | route | ref | network | name          |
            | route       | west      | baserelation | road  | 80  | US:I    | superrelation |

        When I route I should get
            | waypoints | route               | ref       |
            | a,b       | eastbound,eastbound | I 80,I 80 |

    Scenario: Conflict between role and superrelation role
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
            """

        And the ways
            | nodes | name       | highway  | ref   |
            | ab    | eastbound  | motorway | I 80  |

        And the relations
            | type        | way:east | route | ref | network | name         |
            | route       | ab       | road  | 80  | US:I    | baserelation |

        And the relations
            | type        | relation:west  | route | ref | network | name          |
            | route       | baserelation   | road  | 80  | US:I    | superrelation |

        When I route I should get
            | waypoints | route                | ref       |
            | a,b       | eastbound,eastbound  | I 80,I 80 |

    Scenario: Direction only available via superrelation role
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
            """

        And the ways
            | nodes | name       | highway  | ref   |
            | ab    | eastbound  | motorway | I 80  |

        And the relations
            | type        | way:forward | route | ref | network | name         |
            | route       | ab          | road  | 80  | US:I    | baserelation |

        And the relations
            | type        | relation:east  | route | ref | network | name          |
            | route       | baserelation   | road  | 80  | US:I    | superrelation |

        When I route I should get
            | waypoints | route                | ref                   |
            | a,b       | eastbound,eastbound  | I 80 $east,I 80 $east |

    Scenario: Direction only available via superrelation direction
        Given the profile file "car" initialized with
        """
        profile.cardinal_directions = true
        """

        Given the node map
            """
              a----------------b
            """

        And the ways
            | nodes | name       | highway  | ref   |
            | ab    | eastbound  | motorway | I 80  |

        And the relations
            | type        | way:forward | route | ref | network | name         |
            | route       | ab          | road  | 80  | US:I    | baserelation |

        And the relations
            | type        | direction | relation     | route | ref | network | name          |
            | route       | east      | baserelation | road  | 80  | US:I    | superrelation |

        When I route I should get
            | waypoints | route                | ref                   |
            | a,b       | eastbound,eastbound  | I 80 $east,I 80 $east |


#    Scenario: Three levels of indirection
#        Given the node map
#            """
#              a----------------b
#            """
#
#        And the ways
#            | nodes | name       | highway  | ref   |
#            | ab    | eastbound  | motorway | I 80  |
#
#        And the relations
#            | type        | way:forward | route | ref | network | name         |
#            | route       | ab          | road  | 80  | US:I    | baserelation |
#
#        And the relations
#            | type        | relation     | route | ref | network | name           |
#            | route       | baserelation | road  | 80  | US:I    | superrelation1 |
#
#        And the relations
#            | type        | direction | relation       | route | ref | network | name           |
#            | route       | east      | superrelation1 | road  | 80  | US:I    | superrelation2 |
#
#        When I route I should get
#            | waypoints | route                | ref                   |
#            | a,b       | eastbound,eastbound  | I 80 $east,I 80 $east |
