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

    Scenario: Non-Fork on complex intersection - left
        Given the node map
            |   |   |   |   | c |
            | a |   | b |   |   |
            |   | e |   |   | d |

        And the ways
            | nodes  | highway   |
            | abc    | secondary |
            | bd     | tertiary  |
            | eb     | tertiary  |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | abc,abc   | depart,arrive                   |
            | a,d       | abc,bd,bd | depart,turn slight right,arrive |

    Scenario: Non-Fork on complex intersection - right
        Given the node map
            |   | e |   |   | c |
            | a |   | b |   |   |
            |   |   |   |   | d |

        And the ways
            | nodes  | highway   |
            | abd    | secondary |
            | bc     | tertiary  |
            | eb     | tertiary  |

       When I route I should get
            | waypoints | route     | turns                          |
            | a,c       | abd,bc,bc | depart,turn slight left,arrive |
            | a,d       | abd,abd   | depart,arrive                  |

    Scenario: Tripple fork
        Given the node map
            |   |   |   |   |   |   |   |   | c |
            | a |   | b |   | d |   |   |   |   |
            |   |   |   |   |   |   |   |   | e |

        And the ways
            | nodes  | highway   |
            | ab     | secondary |
            | bc     | secondary |
            | bd     | secondary |
            | be     | secondary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,bc,bc | depart,fork slight left,arrive  |
            | a,d       | ab,bd,bd | depart,fork straight,arrive     |
            | a,e       | ab,be,be | depart,fork slight right,arrive |

    Scenario: Tripple fork -- middle obvious
        Given the node map
            |   |   |   |   | c |
            | a |   | b |   | d |
            |   |   |   |   | e |

        And the ways
            | nodes  | highway   |
            | abd    | secondary |
            | bc     | secondary |
            | be     | secondary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | abd,bc,bc | depart,turn slight left,arrive  |
            | a,d       | abd,abd   | depart,arrive                   |
            | a,e       | abd,be,be | depart,turn slight right,arrive |

    Scenario: Don't Fork when leaving Road
        Given the node map
            | a |   | b |   | c |
            |   |   |   |   | d |

        And the ways
            | nodes  | highway   |
            | abc    | secondary |
            | bd     | secondary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | abc,abc   | depart,arrive                   |
            | a,d       | abc,bd,bd | depart,turn slight right,arrive |

     Scenario: Fork on motorway links - don't fork on through
        Given the node map
            | i |   |   |   |   | a |
            | j |   | c | b |   | x |

        And the ways
            | nodes | name | highway       |
            | xb    | xbcj | motorway_link |
            | bc    | xbcj | motorway_link |
            | cj    | xbcj | motorway_link |
            | ci    | off  | motorway_link |
            | ab    | on   | motorway_link |

        When I route I should get
            | waypoints | route      | turns                           |
            | a,j       | on,xbcj    | depart,arrive                   |
            | a,i       | on,off,off | depart,turn slight right,arrive |
