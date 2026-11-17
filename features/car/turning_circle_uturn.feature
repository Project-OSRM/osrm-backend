@routing @car @turning_circle @uturn
Feature: Car - Limited U-turn support at designated turning facilities

    Background:
        Given the profile file "car" initialized with
        """
        profile.speeds.highway.primary = 36
        profile.speed_reduction = 1
        profile.turn_penalty = 0
        profile.oncoming_turn_penalty = 0
        profile.properties.weight_name = 'duration'
        """
        And a grid size of 20 meters
        And the query options
            | annotations | nodes |

    Scenario Outline: Car - U-turn at a through-road <facility> instead of the dead end
        Given the node map
            """
            a 1 2 b c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | aba     |
            | a,c       |              | 80m +-1  | 8s +-0.2  | 8 +-0.2  | abc     |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario: Car - An ordinary through-road node does not permit a U-turn
        Given the node map
            """
            a 1 2 b c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 100m +-1 | 30s +-0.2 | 30 +-0.2 | abcba   |

    Scenario Outline: Car - Tagged dead ends have a five-second U-turn surcharge at <facility>
        Given the node map
            """
            a 1 2 b
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | aba     |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario: Car - Plain dead ends keep the twenty-second U-turn surcharge
        Given the node map
            """
            a 1 2 b
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 26s +-0.2 | 26 +-0.2 | aba     |

    Scenario Outline: Car - Turning facilities do not override a <restriction> restriction
        Given the node map
            """
            a 1 2 b c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        And the relations
            | type        | way:from | node:via | way:to | restriction   |
            | restriction | ab       | b        | <to>   | <restriction> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 100m +-1 | 30s +-0.2 | 30 +-0.2 | abcba   |
        Examples:
            | restriction      | to | facility        |
            | no_u_turn        | ab | turning_circle  |
            | only_straight_on | bc | turning_circle  |
            | no_u_turn        | ab | turning_loop    |
            | only_straight_on | bc | turning_loop    |
            | no_u_turn        | ab | mini_roundabout |
            | only_straight_on | bc | mini_roundabout |

    Scenario Outline: Car - A <facility> cannot reverse a one-way road tagged <oneway>
        Given the node map
            """
            a 1 2 b c
                  d
            """
        And the ways
            | nodes | highway | name   | oneway   |
            | ab    | primary | road   |          |
            | bc    | primary | road   |          |
            | bd    | primary | branch | <oneway> |
        And the nodes
            | node | highway    |
            | d    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 100m +-1 | 30s +-0.2 | 30 +-0.2 | abcba   |

        Examples:
            | facility        | oneway |
            | turning_circle  | yes    |
            | turning_circle  | -1     |
            | turning_loop    | yes    |
            | turning_loop    | -1     |
            | mini_roundabout | yes    |
            | mini_roundabout | -1     |

    Scenario Outline: Car - A <facility> permits a U-turn at a three-way junction
        Given the node map
            """
            a 1 2 b c
                  d
            """
        And the ways
            | nodes | highway | name   |
            | ab    | primary | road   |
            | bc    | primary | road   |
            | bd    | primary | branch |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | aba     |

        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - A longer trip to a turning facility beats a closer plain dead end
        Given the node map
            """
                  d
            a 1 2 b
            |  |
                  c
            |  |
                  e
            """
        And the ways
            | nodes | highway | name     |
            | ab    | primary | main     |
            | bc    | primary | facility |
            | ce    | primary | facility |
            | bd    | primary | branch   |
        And the nodes
            | node | highway    |
            | c    | <facility> |
        When I route I should get
            | waypoints | bearings     | route                            | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | main,facility,facility,main,main | 140m +-1 | 19s +-0.2 | 19 +-0.2 | abcba   |

        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - A <facility> works from both approaches and preserves straight travel
        Given the node map
            """
            a 1 2 b 3 4 c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | aba     |
            | 4,3       | 270,10 90,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | cbc     |
            | a,c       |              | 120m +-1 | 12s +-0.2 | 12 +-0.2 | abc     |
            | c,a       |              | 120m +-1 | 12s +-0.2 | 12 +-0.2 | cba     |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - An internal <facility> survives way compression
        Given the node map
            """
            a 1 2 b c
            """
        And the ways
            | nodes | highway | name |
            | abc   | primary | road |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | aba     |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - A <facility> works at a four-way junction
        Given the node map
            """
                  d
            a 1 2 b c
                  e
            """
        And the ways
            | nodes | highway | name   |
            | ab    | primary | road   |
            | bc    | primary | road   |
            | db    | primary | branch |
            | be    | primary | branch |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | aba     |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - A <facility> respects reverse-approach turn restrictions
        Given the node map
            """
            a b 1 2 c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        And the relations
            | type        | way:from | node:via | way:to | restriction   |
            | restriction | bc       | b        | <to>   | <restriction> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 2,1       | 270,10 90,10 | 100m +-1 | 30s +-0.2 | 30 +-0.2 | cbabc   |
        Examples:
            | facility        | restriction      | to |
            | turning_circle  | no_u_turn        | bc |
            | turning_circle  | only_straight_on | ab |
            | turning_loop    | no_u_turn        | bc |
            | turning_loop    | only_straight_on | ab |
            | mini_roundabout | no_u_turn        | bc |
            | mini_roundabout | only_straight_on | ab |

    Scenario Outline: Car - A custom surcharge of <penalty> seconds is applied exactly once
        Given the profile file "car" initialized with
        """
        profile.speeds.highway.primary = 36
        profile.speed_reduction = 1
        profile.turn_penalty = 0
        profile.oncoming_turn_penalty = 0
        profile.properties.weight_name = 'duration'
        profile.turning_facility_penalty = <penalty>
        """
        Given the node map
            """
            a 1 2 b
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
        And the nodes
            | node | highway        |
            | b    | turning_circle |
        When I route I should get
            | waypoints | bearings     | distance | time          | weight       | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | <time>s +-0.2 | <time> +-0.2 | aba     |
            | a,b       |              | 60m +-1  | 6s +-0.2      | 6 +-0.2      | ab      |
        Examples:
            | penalty | time |
            | 0       | 6    |
            | 5       | 11   |
            | 12      | 18   |
            | 30      | 36   |

    Scenario Outline: Car - Distance weighting preserves asymmetric durations at a <facility>
        Given the profile file "car" initialized with
        """
        profile.speeds.highway.primary = 36
        profile.speed_reduction = 1
        profile.turn_penalty = 0
        profile.oncoming_turn_penalty = 0
        profile.properties.weight_name = 'distance'
        """
        Given the node map
            """
            a 1 2 b c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        And the nodes
            | node | highway    |
            | b    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 60 +-1 | aba     |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - A gate penalty remains additive at a <facility>
        Given the node map
            """
            a 1 2 b
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
        And the nodes
            | node | highway    | barrier |
            | b    | <facility> | gate    |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 71s +-0.2 | 71 +-0.2 | aba     |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - A blocked branch cannot provide a cheap <facility>
        Given the node map
            """
            a 1 2 b c
                  d
            """
        And the ways
            | nodes | highway | name   | access |
            | ab    | primary | road   |        |
            | bc    | primary | road   |        |
            | bd    | primary | branch | no     |
        And the nodes
            | node | highway    |
            | d    | <facility> |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 100m +-1 | 30s +-0.2 | 30 +-0.2 | abcba   |
        Examples:
            | facility        |
            | turning_circle  |
            | turning_loop    |
            | mini_roundabout |

    Scenario Outline: Car - The <side>-hand driving profile retains limited turning-circle costs
        Given the profile file "car" initialized with
        """
        profile.speeds.highway.primary = 36
        profile.speed_reduction = 1
        profile.turn_penalty = 0
        profile.oncoming_turn_penalty = 0
        profile.properties.weight_name = 'duration'
        profile.properties.left_hand_driving = <left>
        """
        Given the node map
            """
            a 1 2 b c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        And the nodes
            | node | highway        |
            | b    | turning_circle |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 11s +-0.2 | 11 +-0.2 | aba     |
        Examples:
            | side  | left  |
            | left  | true  |
            | right | false |

    Scenario Outline: Car - Route choice switches when the facility surcharge exceeds the detour cost (<penalty> seconds)
        Given the profile file "car" initialized with
        """
        profile.speeds.highway.primary = 36
        profile.speed_reduction = 1
        profile.turn_penalty = 0
        profile.oncoming_turn_penalty = 0
        profile.properties.weight_name = 'duration'
        profile.turning_facility_penalty = <penalty>
        """
        Given the node map
            """
            a 1 2 b c
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
            | bc    | primary | road |
        And the nodes
            | node | highway        |
            | b    | turning_circle |
        When I route I should get
            | waypoints | bearings     | distance        | time          | weight       | a:nodes |
            | 1,2       | 90,10 270,10 | <distance>m +-1 | <time>s +-0.2 | <time> +-0.2 | <path>  |
        Examples:
            | penalty | distance | time | path  |
            | 0       | 60       | 6    | aba   |
            | 5       | 60       | 11   | aba   |
            | 20      | 60       | 26   | aba   |
            | 23      | 60       | 29   | aba   |
            | 25      | 100      | 30   | abcba |
            | 30      | 100      | 30   | abcba |

    Scenario Outline: Car - The U-turn angle cost remains additive for <side>-hand driving
        Given the profile file "car" initialized with
        """
        profile.speeds.highway.primary = 36
        profile.speed_reduction = 1
        profile.turn_penalty = 10
        profile.oncoming_turn_penalty = 0
        profile.properties.weight_name = 'duration'
        profile.properties.left_hand_driving = <left>
        """
        Given the node map
            """
            a 1 2 b
            """
        And the ways
            | nodes | highway | name |
            | ab    | primary | road |
        And the nodes
            | node | highway        |
            | b    | turning_circle |
        When I route I should get
            | waypoints | bearings     | distance | time      | weight   | a:nodes |
            | 1,2       | 90,10 270,10 | 60m +-1  | 21s +-0.2 | 21 +-0.2 | aba     |
        Examples:
            | side  | left  |
            | left  | true  |
            | right | false |
