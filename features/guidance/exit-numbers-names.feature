@routing @guidance
Feature: Exit Numbers and Names

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Exit number on the way after the motorway junction
        Given the node map
            """
            a . . b . c . . d
                    ` e . . f
            """

        And the nodes
            | node | highway           |
            | b    | motorway_junction |

        And the ways
            | nodes  | highway       | name     | junction:ref |
            | abcd   | motorway      | MainRoad |              |
            | be     | motorway_link | ExitRamp | 3            |
            | ef     | motorway_link | ExitRamp |              |

       When I route I should get
            | waypoints | route                      | turns                               | exits |
            | a,f       | MainRoad,ExitRamp,ExitRamp | depart,off ramp slight right,arrive | ,3,   |


    Scenario: Exit number on the way, motorway junction node tag missing, multiple numbers
        Given the node map
            """
            a . . b . c . . d
                    ` e . . f
            """

        And the ways
            | nodes  | highway       | name     | junction:ref |
            | abcd   | motorway      | MainRoad |              |
            | be     | motorway_link | ExitRamp | 10;12        |
            | ef     | motorway_link | ExitRamp |              |

       When I route I should get
            | waypoints | route                      | turns                               | exits    |
            | a,f       | MainRoad,ExitRamp,ExitRamp | depart,off ramp slight right,arrive | ,10; 12, |


    Scenario: Exit number on the ways after the motorway junction, multiple exits
        Given the node map
            """
            a . . b . c . . d
                    ` e . . f
                    ` g . . h
            """

        And the nodes
            | node | highway           |
            | b    | motorway_junction |

        And the ways
            | nodes  | highway       | name     | junction:ref |
            | abcd   | motorway      | MainRoad |              |
            | be     | motorway_link | ExitRamp | 3            |
            | ef     | motorway_link | ExitRamp |              |
            | bg     | motorway_link | ExitRamp | 3            |
            | gh     | motorway_link | ExitRamp |              |

       When I route I should get
            | waypoints | route                      | turns                               | exits |
            | a,f       | MainRoad,ExitRamp,ExitRamp | depart,off ramp slight right,arrive | ,3,   |
            | a,h       | MainRoad,ExitRamp,ExitRamp | depart,off ramp right,arrive        | ,3,   |



    # http://www.openstreetmap.org/way/417524818#map=17/37.38663/-121.97972
    Scenario: Exit 393 on Bayshore Freeway
        Given the node map
            """
            a
              ` b
                   ` c
                      .  ` d
                        f     ` e
            """

        And the nodes
            | node | highway           |
            | c    | motorway_junction |

        And the ways
            | nodes  | highway       | name             | junction:ref | oneway | destination                         |
            | abcde  | motorway      | Bayshore Freeway |              | yes    |                                     |
            | cf     | motorway_link |                  | 393          | yes    | Great America Parkway;Bowers Avenue |

       When I route I should get
            | waypoints | route                             | turns                               | exits    | destinations                                                               |
            | a,e       | Bayshore Freeway,Bayshore Freeway | depart,arrive                       | ,        | ,                                                                          |
            | a,f       | Bayshore Freeway,,                | depart,off ramp slight right,arrive | ,393,393 | ,Great America Parkway, Bowers Avenue,Great America Parkway, Bowers Avenue |
