@routing @maxspeed @car
Feature: Car - Acceleration profiles

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Use stoppage penalty at waypoints
        Given the node map
            """
            a 1 2 3 4 5 b
            """

        And the ways
            | nodes | highway | maxspeed:forward    | maxspeed:backward |
            | ab    | trunk   | 60                  | 45                |

        And the query options
            | stoppage_penalty | 5,100   |

        When I route I should get
            | from | to | route | time   |
            | a    | b  | ab,ab | 481.1s |

        When I route I should get
            | from | to | route | time   |
            | b    | a  | ab,ab | 609.9s |

        When I route I should get
            | from | to | route | time |
            | a    | a  | ab,ab | 0s   |

        When I request a travel time matrix I should get
            |   | a     | b     |
            | a | 0     | 481.1 |
            | b | 609.9 | 0     |
