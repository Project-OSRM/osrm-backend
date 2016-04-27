@routing  @guidance
Feature: Fork Instructions

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Fork Same Road Class
        Given the node map
            |   |   |   |   | c |
            | a |   | b |   |   |
            |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | bc     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,bc,bc | depart,fork slight left,arrive  |
            | a,d       | ab,bd,bd | depart,fork slight right,arrive |

    Scenario: Do not fork on link type
        Given the node map
            |   |   |   |   | c |
            | a |   | b |   |   |
            |   |   |   |   | d |

        And the ways
            | nodes  | highway      |
            | abc    | primary      |
            | bd     | primary_link |


       When I route I should get
            | waypoints | route      | turns                           |
            | a,c       | abc,abc    | depart,arrive                   |
            | a,d       | abc,bd,bd  | depart,turn slight right,arrive |

    Scenario: Fork in presence of other roads
        Given the node map
            |   |   |   |   | c |
            | a |   | b |   |   |
            |   | e |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | bc     | primary |
            | bd     | primary |
            | eb     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,bc,bc | depart,fork slight left,arrive  |
            | a,d       | ab,bd,bd | depart,fork slight right,arrive |

    Scenario: Fork Turning Slight Left
        Given the node map
            |   |   |   |   |   | c |
            |   |   |   |   |   |   |
            | a |   | b |   |   |   |
            |   |   |   |   | d |   |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | bc     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,bc,bc | depart,fork slight left,arrive  |
            | a,d       | ab,bd,bd | depart,fork slight right,arrive |

    Scenario: Fork Turning Slight Right
        Given the node map
            |   |   |   |   | c |   |
            | a |   | b |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | bc     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,bc,bc | depart,fork slight left,arrive  |
            | a,d       | ab,bd,bd | depart,fork slight right,arrive |

    Scenario: Do not fork on service
        Given the node map
            |   |   |   |   | c |
            | a |   | b |   |   |
            |   |   |   |   | d |

        And the ways
            | nodes  | highway     |
            | abc    | residential |
            | bd     | service     |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | abc,abc   | depart,arrive                   |
            | a,d       | abc,bd,bd | depart,turn slight right,arrive |

    Scenario: Fork Both Turning Slight Right
        Given the node map
            | a |   | b |   |   |   |
            |   |   |   |   |   | c |
            |   |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | bc     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,bc,bc | depart,fork slight left,arrive  |
            | a,d       | ab,bd,bd | depart,fork slight right,arrive |

    Scenario: Fork Both Turning Slight Left
        Given the node map
            |   |   |   |   |   | c |
            |   |   |   |   |   | d |
            | a |   | b |   |   |   |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | bc     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,bc,bc | depart,fork slight left,arrive  |
            | a,d       | ab,bd,bd | depart,fork slight right,arrive |

    Scenario: Fork Both Turning Slight Right - Unnamed
        Given the node map
            | a |   | b |   |   |   |
            |   |   |   |   |   | c |
            |   |   |   |   |   | d |

        And the ways
            | nodes  | highway | name |
            | ab     | primary |      |
            | bc     | primary |      |
            | bd     | primary |      |

       When I route I should get
            | waypoints | route | turns                           |
            | a,c       | ,,    | depart,fork slight left,arrive  |
            | a,d       | ,,    | depart,fork slight right,arrive |

    Scenario: Fork Both Turning Slight Left - Unnamed
        Given the node map
            |   |   |   |   |   | c |
            |   |   |   |   |   | d |
            | a |   | b |   |   |   |

        And the ways
            | nodes  | highway | name |
            | ab     | primary |      |
            | bc     | primary |      |
            | bd     | primary |      |

       When I route I should get
            | waypoints | route | turns                           |
            | a,c       | ,,    | depart,fork slight left,arrive  |
            | a,d       | ,,    | depart,fork slight right,arrive |

    Scenario: Fork Both Turning Very Slightly Right - Unnamed
        Given the node map
            | a |   | b |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   | c |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | d |

        And the ways
            | nodes  | highway | name |
            | ab     | primary |      |
            | bc     | primary |      |
            | bd     | primary |      |

       When I route I should get
            | waypoints | route | turns                           |
            | a,c       | ,,    | depart,fork slight left,arrive  |
            | a,d       | ,,    | depart,fork slight right,arrive |

    Scenario: Fork Both Turning Very Slightly Right - Unnamed Ramps
        Given the node map
            | a |   | b |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   | c |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | d |

        And the ways
            | nodes  | highway       | name |
            | ab     | motorway_link |      |
            | bc     | motorway_link |      |
            | bd     | motorway_link |      |

       When I route I should get
            | waypoints | route | turns                           |
            | a,c       | ,,    | depart,fork slight left,arrive  |
            | a,d       | ,,    | depart,fork slight right,arrive |

