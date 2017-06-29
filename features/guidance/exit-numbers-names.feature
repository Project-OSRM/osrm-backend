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

