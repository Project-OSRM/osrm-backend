@routing @car @residential
Feature: Car - Residential penalty
The residential_penalty lowers the rate of highway=residential ways, so routes
prefer through roads over residential shortcuts. Speeds and durations do not change.

    Background:
        Given the profile "car"

    Scenario: Car - Residential penalty changes the rate, not the speed
        Then routability should be
            | highway      | maxspeed | surface | width | lane_markings | forw    | backw   | forw_rate | backw_rate |
            | residential  |          |         |       |               | 25 km/h | 25 km/h | 6.3       | 6.3        |
            | unclassified |          |         |       |               | 25 km/h | 25 km/h | 7         | 7          |
            | residential  | 30       |         |       |               | 24 km/h | 24 km/h | 6         | 6          |
            | residential  |          | sand    |       |               | 20 km/h | 20 km/h | 5         | 5          |
            | residential  |          |         | 3     |               | 25 km/h | 25 km/h | 3.5       | 3.5        |
            | residential  |          |         |       | no            | 25 km/h | 25 km/h | 5.2       | 5.2        |

    Scenario: Car - Prefer unclassified road over a slightly shorter residential shortcut
        # unclassified abcd is about 5% longer than residential ad
        Given a grid size of 100 meters
        Given the node map
            """
                b           c
            a                   d
            """

        And the ways
            | nodes | highway      |
            | abcd  | unclassified |
            | ad    | residential  |

        When I route I should get
            | from | to | route     |
            | a    | d  | abcd,abcd |
            | d    | a  | abcd,abcd |

    Scenario: Car - Use residential road when it is much shorter
        # unclassified abcd is about 20% longer than residential ad
        Given a grid size of 100 meters
        Given the node map
            """
              b   c
            a       d
            """

        And the ways
            | nodes | highway      |
            | abcd  | unclassified |
            | ad    | residential  |

        When I route I should get
            | from | to | route |
            | a    | d  | ad,ad |
            | d    | a  | ad,ad |
