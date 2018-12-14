@routing @car @ferry
Feature: Car - Handle ferry routes

    Background:
        Given the profile "car"

    Scenario: Car - Use a ferry route
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | route | bicycle |
            | abc   | primary |       |         |
            | cde   |         | ferry | yes     |
            | efg   | primary |       |         |

        When I route I should get
            | from | to | route           | modes                         |
            | a    | g  | abc,cde,efg,efg | driving,ferry,driving,driving |
            | b    | f  | abc,cde,efg,efg | driving,ferry,driving,driving |
            | e    | c  | cde,cde         | ferry,ferry                   |
            | e    | b  | cde,abc,abc     | ferry,driving,driving         |
            | e    | a  | cde,abc,abc     | ferry,driving,driving         |
            | c    | e  | cde,cde         | ferry,ferry                   |
            | c    | f  | cde,efg,efg     | ferry,driving,driving         |
            | c    | g  | cde,efg,efg     | ferry,driving,driving         |


    Scenario: Car - Use default speeds to calculate duration if no duration given
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | route |
            | abc   | primary |       |
            | cde   |         | ferry |
            | efg   | primary |       |

        When I route I should get
            | from | to | route           | modes                         | speed   | time    |
            | a    | g  | abc,cde,efg,efg | driving,ferry,driving,driving | 12 km/h | 173.4s  |
            | b    | f  | abc,cde,efg,efg | driving,ferry,driving,driving | 9 km/h  | 162.4s  |
            | c    | e  | cde,cde         | ferry,ferry                   | 5 km/h  | 151.4s  |
            | e    | c  | cde,cde         | ferry,ferry                   | 5 km/h  | 151.4s  |

    Scenario: Car - Properly handle simple durations
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | route | duration |
            | abc   | primary |       |          |
            | cde   |         | ferry | 00:01:00 |
            | efg   | primary |       |          |

        When I route I should get
            | from | to | route           | modes                         | speed   | time  |
            | a    | g  | abc,cde,efg,efg | driving,ferry,driving,driving | 24 km/h | 89.4s |
            | b    | f  | abc,cde,efg,efg | driving,ferry,driving,driving | 18 km/h | 78.4s |
            | c    | e  | cde,cde         | ferry,ferry                   | 11 km/h | 67.4s |
            | e    | c  | cde,cde         | ferry,ferry                   | 11 km/h | 67.4s |

    Scenario: Car - Properly handle ISO 8601 durations
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | route | duration |
            | abc   | primary |       |          |
            | cde   |         | ferry | PT1M     |
            | efg   | primary |       |          |

        When I route I should get
            | from | to | route           | modes                         | speed   | time  |
            | a    | g  | abc,cde,efg,efg | driving,ferry,driving,driving | 24 km/h | 89.4s |
            | b    | f  | abc,cde,efg,efg | driving,ferry,driving,driving | 18 km/h | 78.4s |
            | c    | e  | cde,cde         | ferry,ferry                   | 11 km/h | 67.4s |
            | e    | c  | cde,cde         | ferry,ferry                   | 11 km/h | 67.4s |

	@snapping
    Scenario: Car - Snapping when using a ferry
        Given the node map
            """
            a b   c d   e f
            """

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bcde  |         | ferry | 0:10     |
            | ef    | primary |       |          |

        When I route I should get
            | from | to | route     | modes       | time  |
            | c    | d  | bcde,bcde | ferry,ferry | 600s  |

        Given the query options
          | geometries     | geojson                  |
          | overview       | full                     |

        # Note that matching *should* work across unsnappable ferries
        When I match I should get
          | trace | geometry             | duration |
          | abcdef| 1,1,1.000899,1,1.000899,1,1.002697,1,1.002697,1,1.003596,1,1.003596,1,1.005394,1,1.005394,1,1.006293,1 | 610.9      |
