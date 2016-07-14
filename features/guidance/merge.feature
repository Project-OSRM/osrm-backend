@routing  @guidance
Feature: Merging

    Background:
        Given the profile "car"
        And a grid size of 10 meters

    @merge
    Scenario: Merge on Four Way Intersection
        Given the node map
            | d |   |   |   |   |   |   |   |   |   |
            | a |   | b |   |   |   |   |   |   | c |
            | e |   |   |   |   |   |   |   |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | db    | primary |
            | eb    | primary |

       When I route I should get
            | waypoints | route      | turns                       |
            | d,c       | db,abc,abc | depart,turn straight,arrive |
            | e,c       | eb,abc,abc | depart,turn straight,arrive |

    @merge
    Scenario: Merge on Three Way Intersection Right
        Given the node map
            | d |   |   |   |   |   |   |   |   |   |
            | a |   | b |   |   |   |   |   |   | c |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | db    | primary |

       When I route I should get
            | waypoints | route      | turns                       |
            | d,c       | db,abc,abc | depart,turn straight,arrive |

    @merge @negative
    Scenario: Don't Merge on Short-Three Way Intersection Right
        Given the node map
            | d |   |   |   |   |   |   |   |
            | a |   | b |   |   |   |   | c |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | db    | primary |

       When I route I should get
            | waypoints | route      | turns                          |
            | d,c       | db,abc,abc | depart,turn slight left,arrive |


    @merge
    Scenario: Merge on Three Way Intersection Right
        Given the node map
            | a |   | b |   |   |   |   |   |   | c |
            | d |   |   |   |   |   |   |   |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | db    | primary |

       When I route I should get
            | waypoints | route      | turns                       |
            | d,c       | db,abc,abc | depart,turn straight,arrive |

    @merge
    Scenario: Merge onto a turning road
        Given the node map
            |   |   |   |   |   |   | e |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   | d |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   | c |   |   |
            |   |   |   | b |   |   |   |
            | a |   |   |   |   |   | f |

        And the ways
            | nodes | highway     | name |
            | abcde | primary     | road |
            | fd    | residential | in   |

        When I route I should get
            | waypoints | turns                         | route        |
            | f,e       | depart,turn straight,arrive   | in,road,road |
            | f,a       | depart,turn sharp left,arrive | in,road,road |

    @merge
    Scenario: Merge onto a motorway
        Given the node map
            | d |   |   |   |   |   |   |   |   |   |
            | a |   |   | b |   |   |   |   |   | c |

        And the ways
            | nodes | name | highway       | oneway |
            | abc   | A100 | motorway      | yes    |
            | db    |      | motorway_link | yes    |

        When I route I should get
            | waypoints | route      | turns                            |
            | d,c       | ,A100,A100 | depart,merge slight right,arrive |
