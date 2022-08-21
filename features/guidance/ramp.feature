@routing  @guidance
Feature: Ramp Guidance

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Ramp On Through Street Right
        Given the node map
            """
            a b c
              d
            """

        And the ways
            | nodes | highway       |
            | abc   | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,on ramp right,arrive |

    Scenario: Ramp On Through Street Left
        Given the node map
            """
              d
            a b c
            """

        And the ways
            | nodes | highway       |
            | abc   | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route     | turns                         |
            | a,d       | abc,bd,bd | depart,on ramp left,arrive |

    Scenario: Ramp On Through Street Left and Right
        Given the node map
            """
              e
            a b c
              d
            """

        And the ways
            | nodes | highway       |
            | be    | motorway_link |
            | abc   | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,on ramp right,arrive |
            | a,e       | abc,be,be | depart,on ramp left,arrive  |

    Scenario: Ramp On Three Way Intersection Right
        Given the node map
            """
            a b c
              d
            """

        And the ways
            | nodes | highway       |
            | ab    | tertiary      |
            | bc    | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route    | turns                          |
            | a,d       | ab,bd,bd | depart,on ramp right,arrive |

    Scenario: Ramp On Three Way Intersection Right
        Given the node map
            """
                c
            a b
              d
            """

        And the ways
            | nodes | highway       |
            | ab    | tertiary      |
            | bc    | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route    | turns                          |
            | a,d       | ab,bd,bd | depart,on ramp right,arrive |

    Scenario: Ramp Off Though Street
        Given the node map
            """
                    c
            a     b
                  d
            """

        And the ways
            | nodes | highway       | oneway |
            | abc   | tertiary      | yes    |
            | bd    | motorway_link | yes    |

       When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,on ramp right,arrive |
            | a,c       | abc,abc   | depart,arrive                  |

    Scenario: Straight Ramp Off Turning Though Street
        Given the node map
            """
                c
            a b d
            """

        And the ways
            | nodes | highway       |
            | abc   | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,on ramp straight,arrive |
            | a,c       | abc,abc   | depart,arrive                  |

    Scenario: Fork Ramp Off Turning Though Street
        Given the node map
            """
                c
            a b
                d
            """

        And the ways
            | nodes | highway       |
            | abc   | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route     | turns                       |
            | a,d       | abc,bd,bd | depart,on ramp right,arrive |
            | a,c       | abc,abc   | depart,arrive               |

    Scenario: Fork Ramp
        Given the node map
            """
                c
            a b
                d
            """

        And the ways
            | nodes | highway       |
            | ab    | tertiary      |
            | bc    | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route    | turns                          |
            | a,d       | ab,bd,bd | depart,on ramp right,arrive    |
            | a,c       | ab,bc    | depart,arrive                  |

    Scenario: Fork Slight Ramp
        Given the node map
            """
                  c
            a b
                  d
            """

        And the ways
            | nodes | highway       |
            | ab    | tertiary      |
            | bc    | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route    | turns                                 |
            | a,d       | ab,bd,bd | depart,on ramp slight right,arrive    |
            | a,c       | ab,bc    | depart,arrive                         |

    Scenario: Fork Slight Ramp on Through Street
        Given the node map
            """
                  c
            a b
                  d
            """

        And the ways
            | nodes | highway       |
            | abc   | tertiary      |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route     | turns                              |
            | a,d       | abc,bd,bd | depart,on ramp slight right,arrive |
            | a,c       | abc,abc   | depart,arrive                      |

    Scenario: Fork Slight Ramp on Obvious Through Street
        Given the node map
            """
                  c
            a b
                  d
            """

        And the ways
            | nodes | highway       |
            | abc   | primary       |
            | bd    | motorway_link |

       When I route I should get
            | waypoints | route     | turns                                 |
            | a,d       | abc,bd,bd | depart,on ramp slight right,arrive |
            | a,c       | abc,abc   | depart,arrive                         |

    Scenario: Two Ramps Joining into common Motorway
        Given the node map
            """
            a
                c d
            b
            """

        And the ways
            | nodes | highway       |
            | ac    | motorway_link |
            | bc    | motorway_link |
            | cd    | motorway      |

        When I route I should get
            | waypoints | route | turns         |
            | a,d       | ac,cd | depart,arrive |
            | b,d       | bc,cd | depart,arrive |

    Scenario: Two Ramps Joining into common Motorway Unnamed
        Given the node map
            """
            a
                c d
            b
            """

        And the ways
            | nodes | highway       | name |
            | ac    | motorway_link |      |
            | bc    | motorway_link |      |
            | cd    | motorway      |      |

        When I route I should get
            | waypoints | route    | turns         |
            | a,d       | ,        | depart,arrive |
            | b,d       | ,        | depart,arrive |

    Scenario: Ferry Onto A Ramp
        Given the node map
            """
                                d - e - g
                                |
            a - b ~ ~ ~ ~ ~ ~ ~ c
                                  ` f
            """

        And the ways
            | nodes | highway       | route | name                       | ref |
            | ab    | primary       |       | boarding                   |     |
            | bc    |               | ferry | boaty mc boatface          | m2  |
            | cf    |               | ferry | boaty mc boatface          |     |
            | cd    |               | ferry | boaty mc boatface's cousin |     |
            | de    | motorway_link |       |                            |     |

        When I route I should get
            | waypoints | route                        |
            | a,e       | boarding,boaty mc boatface,, |
