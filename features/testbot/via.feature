@routing @testbot @via
Feature: Via points

    Background:
        Given the profile "testbot"

    Scenario: Simple via point
        Given the node map
            | a | b | c |

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | waypoints | route   |
            | a,b,c     | abc,abc |
            | c,b,a     | abc,abc |

    Scenario: Via point at a dead end
        Given the node map
            | a | b | c |
            |   | d |   |

        And the ways
            | nodes |
            | abc   |
            | bd    |

        When I route I should get
            | waypoints | route            |
            | a,d,c     | abc,bd,bd,bd,abc |
            | c,d,a     | abc,bd,bd,bd,abc |

    Scenario: Multiple via points
        Given the node map
            | a |   |   |   | e | f | g |   |
            |   | b | c | d |   |   |   | h |

        And the ways
            | nodes |
            | ae    |
            | ab    |
            | bcd   |
            | de    |
            | efg   |
            | gh    |
            | dh    |

        When I route I should get
            | waypoints | route                    |
            | a,c,f     | ab,bcd,bcd,de,efg        |
            | a,c,f,h   | ab,bcd,bcd,de,efg,efg,gh |


    Scenario: Duplicate via point
        Given the node map
            | x |   |   |   |   |   |
            | a | 1 | 2 | 3 | 4 | b |
            |   |   |   |   |   |   |

        And the ways
            | nodes |
            | xa    |
            | ab    |

        When I route I should get
            | waypoints | route | turns                |
            | 1,1,4     | ab,ab | head,via,destination |

    Scenario: Via points on ring of oneways
    # xa it to avoid only having a single ring, which cna trigger edge cases
        Given the node map
            | x |   |   |   |   |   |   |
            | a | 1 | b | 2 | c | 3 | d |
            | f |   |   |   |   |   | e |

        And the ways
            | nodes | oneway |
            | xa    |        |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fa    | yes    |

        When I route I should get
            | waypoints | route                               | distance  | turns                                                                                                    |
            | 1,3       | ab,bc,cd                            |  400m +-1 | head,straight,straight,destination                                                                       |
            | 3,1       | cd,de,ef,fa,ab                      | 1000m +-1 | head,straight,straight,straight,right,destination                                                        |
            | 1,2,3     | ab,bc,bc,cd                         |  400m +-1 | head,straight,via,straight,destination                                                                   |
            | 1,3,2     | ab,bc,cd,cd,de,ef,fa,ab,bc          | 1600m +-1 | head,straight,straight,via,straight,straight,straight,right,straight,destination                         |
            | 3,2,1     | cd,de,ef,fa,ab,bc,bc,cd,de,ef,fa,ab | 2400m +-1 | head,straight,straight,straight,right,straight,via,straight,straight,straight,straight,right,destination |

    Scenario: Via points on ring on the same oneway
    # xa it to avoid only having a single ring, which cna trigger edge cases
        Given the node map
            | x |   |   |   |   |
            | a | 1 | 2 | 3 | b |
            | d |   |   |   | c |

        And the ways
            | nodes | oneway |
            | xa    |        |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                         | distance  | turns                                                                                  |
            | 1,3       | ab                            | 200m +-1  | head,destination                                                                       |
            | 3,1       | ab,bc,cd,da,ab                | 800m +-1  | head,straight,straight,straight,right,destination                                      |
            | 1,2,3     | ab,ab                         | 200m +-1  | head,via,destination                                                                   |
            | 1,3,2     | ab,ab,bc,cd,da,ab             | 1100m +-1 | head,via,straight,straight,straight,right,destination                                  |
            | 3,2,1     | ab,bc,cd,da,ab,ab,bc,cd,da,ab | 1800m     | head,straight,straight,straight,right,via,straight,straight,straight,right,destination |

    # See issue #1896
    Scenario: Via point at a dead end with oneway
        Given the node map
            | a | b | c |
            |   | d |   |
            |   | e |   |

        And the ways
            | nodes | oneway |
            | abc   |  no    |
            | bd    |  no    |
            | de    |  yes   |

        When I route I should get
            | waypoints | route            |
            | a,d,c     | abc,bd,bd,bd,abc |
            | c,d,a     | abc,bd,bd,bd,abc |

    # See issue #1896
    Scenario: Via point at a dead end with barrier
        Given the profile "car"
        Given the node map
            | a | b | c |
            |   | 1 |   |
            |   | d |   |
            |   |   |   |
            |   |   |   |
            | f | e |   |

        And the nodes
            | node | barrier |
            | d    | bollard |

        And the ways
            | nodes |
            | abc   |
            | bd    |
            | afed  |

        When I route I should get
            | waypoints | route            |
            | a,1,c     | abc,bd,bd,bd,abc |
            | c,1,a     | abc,bd,bd,bd,abc |

    Scenario: Via points on ring on the same oneway, forces one of the vertices to be top node
        Given the node map
            | a | 1 | 2 | b |
            | 8 |   |   | 3 |
            | 7 |   |   | 4 |
            | d | 6 | 5 | c |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                      | distance   | turns                                                                        |
            | 2,1       | ab,bc,cd,da,ab             | 1100m +-1  | head,straight,straight,straight,straight,destination                         |
            | 4,3       | bc,cd,da,ab,bc             | 1100m +-1  | head,straight,straight,straight,straight,destination                         |
            | 6,5       | cd,da,ab,bc,cd             | 1100m +-1  | head,straight,straight,straight,straight,destination                         |
            | 8,7       | da,ab,bc,cd,da             | 1100m +-1  | head,straight,straight,straight,straight,destination                         |

    Scenario: Multiple Via points on ring on the same oneway, forces one of the vertices to be top node
        Given the node map
            | a | 1 | 2 | 3 | b |
            |   |   |   |   | 4 |
            |   |   |   |   | 5 |
            |   |   |   |   | 6 |
            | d | 9 | 8 | 7 | c |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                         | distance     | turns                                                                                        |
            | 3,2,1     | ab,bc,cd,da,ab,ab,bc,cd,da,ab | 3000m +-1    | head,straight,straight,straight,straight,via,straight,straight,straight,straight,destination |
            | 6,5,4     | bc,cd,da,ab,bc,bc,cd,da,ab,bc | 3000m +-1    | head,straight,straight,straight,straight,via,straight,straight,straight,straight,destination |
            | 9,8,7     | cd,da,ab,bc,cd,cd,da,ab,bc,cd | 3000m +-1    | head,straight,straight,straight,straight,via,straight,straight,straight,straight,destination |
