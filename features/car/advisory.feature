@routing @maxspeed @car
Feature: Car - Max speed restrictions
OSRM will use 4/5 of the projected free-flow speed.

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Advisory speed overwrites maxspeed
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway       | maxspeed | maxspeed:advisory |
            | ab    | residential   | 90       | 45                |
            | bc    | residential   |          | 45                |

        When I route I should get
            | from | to | route | speed        |
            | a    | b  | ab,ab | 36 km/h +- 1 |
            | b    | c  | bc,bc | 36 km/h +- 1 |

    Scenario: Car - Advisory speed overwrites forward maxspeed
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway       | maxspeed:forward | maxspeed:advisory:forward |
            | ab    | residential   | 90               | 45                        |
            | bc    | residential   |                  | 45                        |

        When I route I should get
            | from | to | route | speed        |
            | a    | b  | ab,ab | 36 km/h +- 1 |
            | b    | c  | bc,bc | 36 km/h +- 1 |

    Scenario: Car - Advisory speed overwrites backwards maxspeed
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway       | maxspeed:backward | maxspeed:advisory:backward |
            | ab    | residential   | 90                | 45                         |
            | bc    | residential   |                   | 45                         |

        When I route I should get
            | from | to | route | speed        |
            | b    | a  | ab,ab | 36 km/h +- 1 |
            | c    | b  | bc,bc | 36 km/h +- 1 |

    Scenario: Car - Advisory speed overwrites backwards maxspeed
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway       | maxspeed:backward | maxspeed:advisory:backward |
            | ab    | residential   |                   | 45                         |
            | bc    | residential   | 90                | 45                         |
            | cd    | residential   |                   | 45                         |

        When I route I should get
            | from | to | route | speed        |
            | c    | b  | bc,bc | 36 km/h +- 1 |
            | d    | c  | cd,cd | 36 km/h +- 1 |

    Scenario: Car - Directional advisory speeds play nice with eachother
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | highway       | maxspeed:advisory | maxspeed:advisory:forward | maxspeed:advisory:backward |
            | ab    | residential   | 90                | 45                        | 60                         |
            | bc    | residential   | 90                | 60                        | 45                         |

        When I route I should get
            | from | to | route | speed        |
            | a    | b  | ab,ab | 36 km/h +- 1 |
            | b    | a  | ab,ab | 48 km/h +- 1 |
            | b    | c  | bc,bc | 48 km/h +- 1 |
            | c    | b  | bc,bc | 36 km/h +- 1 |


