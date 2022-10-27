@routing  @guidance
Feature: Continue Instructions

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Road turning left
        Given the node map
            """
                c
            a - b-d
            """

        And the ways
            | nodes  | highway |
            | abc    | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route       | turns                       |
            | a,c       | abc,abc,abc | depart,continue left,arrive |
            | a,d       | abc,bd,bd   | depart,turn straight,arrive |

    Scenario: Road turning left, Suffix changes
        Given the node map
            """
                c
            a - b-d
            """

        And the ways
            | nodes  | highway | name                    |
            | ab     | primary | North Capitol Northeast |
            | bc     | primary | North Capitol Northwest |
            | bd     | primary | some random street      |

       When I route I should get
            | waypoints | route                                                                   | turns                       |
            | a,c       | North Capitol Northeast,North Capitol Northwest,North Capitol Northwest | depart,continue left,arrive |

    Scenario: Road turning left, Suffix changes, no-spaces
        Given the node map
            """
                c
            a - b-d
            """

        And the ways
            | nodes  | highway | name                   |
            | ab     | primary | North CapitolNortheast |
            | bc     | primary | North CapitolNorthwest |
            | bd     | primary | some random street     |

       When I route I should get
            | waypoints | route                                                                | turns                       |
            | a,c       | North CapitolNortheast,North CapitolNorthwest,North CapitolNorthwest | depart,continue left,arrive |

    Scenario: Road turning left and straight
        Given the node map
            """
                c
            a - b-d
            """

        And the ways
            | nodes  | highway | name |
            | abc    | primary | road |
            | bd     | primary | road |

       When I route I should get
            | waypoints | route          | turns                       |
            | a,c       | road,road,road | depart,continue left,arrive |
            | a,d       | road,road      | depart,arrive               |

    Scenario: Road turning left and straight
        Given the node map
            """
                c
            a - b-d
                e
            """

        And the ways
            | nodes  | highway | name |
            | abc    | primary | road |
            | bd     | primary | road |
            | be     | primary | road |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,c       | road,road,road | depart,continue left,arrive  |
            | a,d       | road,road      | depart,arrive                |
            | a,e       | road,road,road | depart,continue right,arrive |

    Scenario: Road turning right
        Given the node map
            """
            a - b-d
                c
            """

        And the ways
            | nodes  | highway |
            | abc    | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route       | turns                        |
            | a,c       | abc,abc,abc | depart,continue right,arrive |
            | a,d       | abc,bd,bd   | depart,turn straight,arrive  |

    Scenario: Road turning slight left
        Given the node map
            """
                    c
                  /
            a - b
                 `d
            """

        And the ways
            | nodes  | highway |
            | abc    | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route       | turns                       |
            | a,c       | abc,abc,abc | depart,continue left,arrive |
            | a,d       | abc,bd,bd   | depart,turn right,arrive    |

    Scenario: Road turning slight right
        Given the node map
            """
                 ,d
            a - b
                  \
                    c
            """

        And the ways
            | nodes  | highway |
            | abc    | primary |
            | bd     | primary |

       When I route I should get
            | waypoints | route       | turns                        |
            | a,c       | abc,abc,abc | depart,continue right,arrive |
            | a,d       | abc,bd,bd   | depart,turn left,arrive      |

    Scenario: Road Loop
       Given the node map
           """
               f - e
               |   |
           a - b-g |
               |   |
               c - d
           """

       And the ways
          | nodes   | highway |
          | abcdefb | primary |
          | bg      | primary |

       When I route I should get
          | waypoints | route                   | turns                        |
          | a,c       | abcdefb,abcdefb,abcdefb | depart,continue right,arrive |
          | a,f       | abcdefb,abcdefb,abcdefb | depart,continue left,arrive  |
          | a,d       | abcdefb,abcdefb,abcdefb | depart,continue right,arrive |
          # continuing right here, since the turn to the left is more expensive
          | a,e       | abcdefb,abcdefb,abcdefb | depart,continue right,arrive |

    Scenario: End-Of-Road Continue
        Given the node map
            """
            a - b - c
                |
                d - e
                |
                f
            """

        And the ways
            | nodes | highway | name |
            | abc   | primary | road |
            | bdf   | primary | road |
            | ed    | primary | turn |


        When I route I should get
            | waypoints | route               | turns                                  |
            | e,a       | turn,road,road,road | depart,turn right,continue left,arrive |
