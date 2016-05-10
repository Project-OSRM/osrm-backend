@routing  @guidance
Feature: End Of Road Instructions

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: End of Road with through street
        Given the node map
            |   |   | c |
            | a |   | b |
            |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route      | turns                           |
            | a,c       | ab,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | ab,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with three streets
        Given the node map
            |   |   | c |
            | a |   | b |
            |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,cb,cb | depart,end of road left,arrive  |
            | a,d       | ab,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with three streets, slightly angled
        Given the node map
            | a |   |   |   |   | c |
            |   |   |   |   |   | b |
            |   |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,cb,cb | depart,end of road left,arrive  |
            | a,d       | ab,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with three streets, slightly angled
        Given the node map
            |   |   |   |   |   | c |
            |   |   |   |   |   | b |
            | a |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,cb,cb | depart,end of road left,arrive  |
            | a,d       | ab,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with through street, slightly angled
        Given the node map
            | a |   |   |   |   | c |
            |   |   |   |   |   | b |
            |   |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route      | turns                           |
            | a,c       | ab,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | ab,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with through street, slightly angled
        Given the node map
            |   |   |   |   |   | c |
            |   |   |   |   |   | b |
            | a |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route      | turns                           |
            | a,c       | ab,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | ab,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with two ramps - prefer ramp over end of road
        Given the node map
            |   |   | c |
            | a |   | b |
            |   |   | d |

        And the ways
            | nodes  | highway       |
            | ab     | primary       |
            | bc     | motorway_link |
            | bd     | motorway_link |

       When I route I should get
            | waypoints | route    | turns                       |
            | a,c       | ab,bc,bc | depart,on ramp left,arrive  |
            | a,d       | ab,bd,bd | depart,on ramp right,arrive |

