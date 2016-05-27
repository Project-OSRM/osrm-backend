@routing  @guidance
Feature: Slipways and Dedicated Turn Lanes

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: Turn Instead of Ramp
        Given the node map
            |   |   |   |   | e |   |
            | a | b |   |   | c | d |
            |   |   |   | h |   |   |
            |   |   |   |   |   |   |
            |   |   |   | 1 |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   | f |   |
            |   |   |   |   |   |   |
            |   |   |   |   | g |   |

        And the ways
            | nodes | highway    | name   |
            | abcd  | trunk      | first  |
            | bhf   | trunk_link |        |
            | ecfg  | primary    | second |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abcd     | ecfg   | c        | no_right_turn |

       When I route I should get
            | waypoints | route               | turns                           |
            | a,g       | first,second,second | depart,turn right,arrive        |
            | a,1       | first,,             | depart,turn slight right,arrive |

    Scenario: Turn Instead of Ramp
        Given the node map
            |   |   |   |   | e |   |
            | a | b |   |   | c | d |
            |   |   |   | h |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   | f |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   | g |   |

        And the ways
            | nodes | highway       | name   |
            | abcd  | motorway      | first  |
            | bhf   | motorway_link |        |
            | efg   | primary       | second |

       When I route I should get
            | waypoints | route                | turns                                                 |
            | a,g       | first,,second,second | depart,off ramp slight right,merge slight left,arrive |

    Scenario: Inner city expressway with on road
        Given the node map
            | a | b |   |   |   | c |
            |   |   |   |   | f |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   | d |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   | e |

        And the ways
            | nodes | highway      | name  |
            | abc   | primary      | road  |
            | bfd   | trunk_link   |       |
            | cde   | trunk        | trunk |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cde    | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                    |
            | a,e       | road,trunk,trunk     | depart,turn right,arrive |


    Scenario: Slipway Round U-Turn
        Given the node map
            | a |   | f |
            |   |   |   |
            | b |   | e |
            |   |   |   |
            |   |   |   |
            |   | g |   |
            |   |   |   |
            | c |   | d |

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |

    Scenario: Slipway Steep U-Turn
        Given the node map
            | a |   | f |
            |   |   |   |
            | b |   | e |
            |   | g |   |
            |   |   |   |
            |   |   |   |
            | c |   | d |

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |
