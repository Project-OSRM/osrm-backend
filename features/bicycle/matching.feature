@match @bicycle
Feature: Bike - Map Matching Road Width

    Background:
        Given the profile "bicycle"
        Given the query options
            | geometries | geojson |

    Scenario: Bicycle oneway exception keeps both physical road widths
        Given a grid size of 4 meters
        Given the node map
            """
            a b c d


            1 2 3 4
            e f g h
            """

        And the ways
            | nodes | highway  | oneway | oneway:bicycle | width:forward | width:backward | width |
            | abcd  | primary  | no     | yes            | 13            | 13             |       |
            | efgh  | primary  | no     |                |               |                | 6     |

        When I match I should get
            | trace | matchings |
            | 1234  | abcd      |

    Scenario: Physical oneway width applies with bicycle exception
        Given a grid size of 2 meters
        Given the node map
            """
            a b c d


            1 2 3 4

            e f g h
            """

        And the ways
            | nodes | highway | oneway | oneway:bicycle | lanes | width:lanes:forward |
            | abcd  | primary | yes    | no             | 2     | 5\|5                |
            | efgh  | primary | no     |                |       |                     |

        When I match I should get
            | trace | matchings |
            | 1234  | abcd      |
