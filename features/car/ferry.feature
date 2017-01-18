@routing @car @ferry
Feature: Car - Handle ferry routes

    Background:
        Given the profile "car.lua"

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
            | from | to | route           | modes                         | speed   |
            | a    | g  | abc,cde,efg,efg | driving,ferry,driving,driving | 23 km/h |
            | b    | f  | abc,cde,efg,efg | driving,ferry,driving,driving | 18 km/h |
            | c    | e  | cde,cde         | ferry,ferry                   | 11 km/h |
            | e    | c  | cde,cde         | ferry,ferry                   | 11 km/h |

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
            | from | to | route           | modes                         | speed   |
            | a    | g  | abc,cde,efg,efg | driving,ferry,driving,driving | 23 km/h |
            | b    | f  | abc,cde,efg,efg | driving,ferry,driving,driving | 18 km/h |
            | c    | e  | cde,cde         | ferry,ferry                   | 11 km/h |
            | e    | c  | cde,cde         | ferry,ferry                   | 11 km/h |
