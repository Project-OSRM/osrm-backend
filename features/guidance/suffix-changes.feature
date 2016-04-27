@routing  @guidance
Feature: Suppress New Names on dedicated Suffices

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Suffix To Suffix
        Given the node map
            | a |   | b |   | c |

        And the ways
            | nodes  | name |
            | ab     | 42 N |
            | bc     | 42 S |

       When I route I should get
            | waypoints | route     | turns         |
            | a,c       | 42 N,42 S | depart,arrive |

    Scenario: Suffix To Suffix (long)
        Given the node map
            | a |   | b |   | c |

        And the ways
            | nodes  | name      |
            | ab     | 42 Road   |
            | bc     | 42 Street |

       When I route I should get
            | waypoints | route             | turns         |
            | a,c       | 42 Road,42 Street | depart,arrive |

    Scenario: Suffix To Suffix (connected)
        Given the node map
            | a |   | b |   | c |

        And the ways
            | nodes  | name       |
            | ab     | Mainstreet |
            | bc     | Mainroad   |

       When I route I should get
            | waypoints | route               | turns         |
            | a,c       | Mainstreet,Mainroad | depart,arrive |


    Scenario: Suffix To Suffix - Turn
        Given the node map
            | a |   | b |   | c |
            |   |   | d |   |   |

        And the ways
            | nodes  | name |
            | ab     | 42 N |
            | bc     | 42 S |
            | bd     | 42 E |

       When I route I should get
            | waypoints | route          | turns                    |
            | a,c       | 42 N,42 S      | depart,arrive            |
            | a,d       | 42 N,42 E,42 E | depart,turn right,arrive |

    Scenario: Suffix To No Suffix
        Given the node map
            | a |   | b |   | c |

        And the ways
            | nodes  | name |
            | ab     | 42 N |
            | bc     | 42   |

       When I route I should get
            | waypoints | route   | turns         |
            | a,c       | 42 N,42 | depart,arrive |

    Scenario: No Suffix To Suffix
        Given the node map
            | a |   | b |   | c |

        And the ways
            | nodes  | name |
            | ab     | 42   |
            | bc     | 42 S |

       When I route I should get
            | waypoints | route   | turns         |
            | a,c       | 42,42 S | depart,arrive |

