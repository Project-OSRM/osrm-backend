@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters

    Scenario: Ramp Exit Right
        Given the node map
            | a | b | c | d | e |
            |   |   |   | f | g |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | bfg    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                             |
            | a,e       | abcde, abcde    | depart, arrive                    |
            | a,g       | abcde, bfg, bfg | depart, ramp-slight-right, arrive |

    Scenario: Ramp Exit Right Curved Right
        Given the node map
            | a | b | c |   |   |
            |   |   | f | d |   |
            |   |   |   | g | e |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | bfg    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                             |
            | a,e       | abcde, abcde    | depart, arrive                    |
            | a,g       | abcde, bfg, bfg | depart, ramp-slight-right, arrive |

    Scenario: Ramp Exit Right Curved Left
        Given the node map
            |   |   |   |   | e |
            |   |   |   | d | g |
            | a | b | c | f |   |


        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | cfg    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                             |
            | a,e       | abcde, abcde    | depart, arrive                    |
            | a,g       | abcde, cfg, cfg | depart, ramp-slight-right, arrive |


    Scenario: Ramp Exit Left
        Given the node map
            |   |   |   | f | g |
            | a | b | c | d | e |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | bfg    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                            |
            | a,e       | abcde, abcde    | depart, arrive                   |
            | a,g       | abcde, bfg, bfg | depart, ramp-slight-left, arrive |

    Scenario: Ramp Exit Left Curved Left
        Given the node map
            |   |   |   | g | e |
            |   |   | f | d |   |
            | a | b | c |   |   |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | bfg    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                            |
            | a,e       | abcde, abcde    | depart, arrive                   |
            | a,g       | abcde, bfg, bfg | depart, ramp-slight-left, arrive |

    Scenario: Ramp Exit Left Curved Right
        Given the node map
            | a | b | c | f |   |
            |   |   |   | d | g |
            |   |   |   |   | e |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | cfg    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                            |
            | a,e       | abcde, abcde    | depart, arrive                   |
            | a,g       | abcde, cfg, cfg | depart, ramp-slight-left, arrive |

    Scenario: On Ramp Right
        Given the node map
            | a | b | c | d | e |
            | f | g |   |   |   |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | fgd    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                             |
            | a,e       | abcde, abcde    | depart, arrive                    |
            | f,e       | abcde, fgd, fgd | depart, merge-slight-left, arrive |

    Scenario: On Ramp Left
        Given the node map
            | f | g |   |   |   |
            | a | b | c | d | e |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | fgd    | motorway_link |

       When I route I should get
            | waypoints | route           | turns                              |
            | a,e       | abcde, abcde    | depart, arrive                     |
            | f,e       | abcde, fgd, fgd | depart, merge-slight-right, arrive |

    Scenario: Highway Fork
        Given the node map
            |   |   |   |   | d | e |
            | a | b | c |   |   |   |
            |   |   |   |   | f | g |

        And the ways
            | nodes  | highway  |
            | abcde  | motorway |
            | cfg    | motorway |

       When I route I should get
            | waypoints | route               | turns                      |
            | a,e       | abcde, abcde, abcde | depart, fork-left, arrive  |
            | a,g       | abcde, cfg, cfg     | depart, fork-right, arrive |

     Scenario: Fork After Ramp
       Given the node map
            |   |   |   |   | d | e |
            | a | b | c |   |   |   |
            |   |   |   |   | f | g |

        And the ways
            | nodes  | highway       |
            | abc    | motorway_link |
            | cde    | motorway      |
            | cfg    | motorway      |

       When I route I should get
            | waypoints | route         | turns                      |
            | a,e       | abc, cde, cde | depart, fork-left, arrive  |
            | a,g       | abc, cfg, cfg | depart, fork-right, arrive |

     Scenario: On And Off Ramp Right
       Given the node map
            | a | b |   | c |   | d | e |
            | f | g |   |   |   | h | i |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | fgc    | motorway_link |
            | chi    | motorway_link |

       When I route I should get
            | waypoints | route             | turns                             |
            | a,e       | abcde, abcde      | depart, arrive                    |
            | f,e       | fgc, abcde, abcde | depart, merge-slight-left, arrive |
            | a,i       | abcde, chi, chi   | depart, ramp-slight-right, arrive |
            | f,i       | fgc, chi, chi     | depart, turn-slight-right, arrive |

    Scenario: On And Off Ramp Left
       Given the node map
            | f | g |   |   |   | h | i |
            | a | b |   | c |   | d | e |

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | fgc    | motorway_link |
            | chi    | motorway_link |

       When I route I should get
            | waypoints | route             | turns                              |
            | a,e       | abcde, abcde      | depart, arrive                     |
            | f,e       | fgc, abcde, abcde | depart, merge-slight-right, arrive |
            | a,i       | abcde, chi, chi   | depart, ramp-slight-left, arrive   |
            | f,i       | fgc, chi, chi     | depart, turn-slight-left, arrive   |

