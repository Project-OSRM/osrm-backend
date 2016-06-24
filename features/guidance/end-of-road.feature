@routing  @guidance
Feature: End Of Road Instructions

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: End of Road with through street
        Given the node map
            |   |   | c |
            | a | e | b |
            |   | f | d |

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | cbd    | primary |
            | ef     | primary |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,c       | aeb,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | aeb,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with three streets
        Given the node map
            |   |   | c |
            | a | e | b |
            |   | f | d |

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | cb     | primary |
            | bd     | primary |
            | ef     | primary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | aeb,cb,cb | depart,end of road left,arrive  |
            | a,d       | aeb,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with three streets, slightly angled
        Given the node map
            | a | e |   |   |   | c |
            |   | f |   |   |   | b |
            |   |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | cb     | primary |
            | bd     | primary |
            | ef     | primary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | aeb,cb,cb | depart,end of road left,arrive  |
            | a,d       | aeb,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with three streets, slightly angled
        Given the node map
            |   |   |   |   |   | c |
            |   | f |   |   |   | b |
            | a | e |   |   |   | d |

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | ef     | primary |
            | cb     | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route     | turns                           |
            | a,c       | aeb,cb,cb | depart,end of road left,arrive  |
            | a,d       | aeb,bd,bd | depart,end of road right,arrive |

    Scenario: End of Road with through street, slightly angled
        Given the node map
            | a | e |   |   |   | c |
            |   | f |   |   |   | b |
            |   |   |   |   |   | d |

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | ef     | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,c       | aeb,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | aeb,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with through street, slightly angled
        Given the node map
            |   |   |   |   |   | c |
            |   | f |   |   |   | b |
            | a | e |   |   |   | d |

        And the ways
            | nodes  | highway |
            | aeb    | primary |
            | ef     | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route       | turns                           |
            | a,c       | aeb,cbd,cbd | depart,end of road left,arrive  |
            | a,d       | aeb,cbd,cbd | depart,end of road right,arrive |

    Scenario: End of Road with two ramps - prefer ramp over end of road
        Given the node map
            |   |   | c |
            | a | e | b |
            |   | f | d |

        And the ways
            | nodes  | highway       |
            | aeb    | primary       |
            | ef     | primary       |
            | bc     | motorway_link |
            | bd     | motorway_link |

       When I route I should get
            | waypoints | route     | turns                       |
            | a,c       | aeb,bc,bc | depart,on ramp left,arrive  |
            | a,d       | aeb,bd,bd | depart,on ramp right,arrive |

