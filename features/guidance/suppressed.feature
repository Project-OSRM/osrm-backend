@routing  @guidance
Feature: Suppressed Turns

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Do not announce passing a exit ramp
        Given the node map
            """
            a-b-c-d-e
              \---f-g
            """

        And the ways
            | nodes  | highway       |
            | abcde  | motorway      |
            | bfg    | motorway_link |

       When I route I should get
            | waypoints | route         | turns         |
            | a,e       | abcde,abcde   | depart,arrive |

    Scenario: Do not announce reference changes
        Given the node map
            """
            a-b-c-d-e-f
            """

        And the ways
            | nodes | highway  | name     | ref   |
            | ab    | motorway | highway  | A1    |
            | bc    | motorway | highway  | A1,A2 |
            | cd    | motorway | highway  | A2    |
            | de    | motorway | highway  |       |
            | ef    | motorway | highway  | A1    |

        When I route I should get
            | waypoints | route                     | turns         | ref    |
            | a,f       | highway,highway           | depart,arrive | A1,A1  |


    Scenario: Don't Announce Turn on following major road class -- service
        Given the node map
            """
            a-b-d
                c
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | service |

        When I route I should get
            | waypoints | route   | turns         |
            | a,c       | abc,abc | depart,arrive |

    Scenario: Don't Announce Turn on following major road class -- residential
        Given the node map
            """
            a-b-d
                c
            """

        And the ways
            | nodes | highway     |
            | abc   | primary     |
            | bd    | residential |

        When I route I should get
            | waypoints | route     | turns                       |
            | a,c       | abc,abc   | depart,arrive               |
            | a,d       | abc,bd,bd | depart,turn straight,arrive |
