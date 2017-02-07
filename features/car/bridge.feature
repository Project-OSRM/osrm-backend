@routing @car @bridge
Feature: Car - Handle driving

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
            | nodes | highway | bridge  | bicycle |
            | abc   | primary |         |         |
            | cde   |         | movable | yes     |
            | efg   | primary |         |         |

        When I route I should get
            | from | to | route           | modes                           |
            | a    | g  | abc,cde,efg,efg | driving,driving,driving,driving |
            | b    | f  | abc,cde,efg,efg | driving,driving,driving,driving |
            | e    | c  | cde,cde         | driving,driving                 |
            | e    | b  | cde,abc,abc     | driving,driving,driving         |
            | e    | a  | cde,abc,abc     | driving,driving,driving         |
            | c    | e  | cde,cde         | driving,driving                 |
            | c    | f  | cde,efg,efg     | driving,driving,driving         |
            | c    | g  | cde,efg,efg     | driving,driving,driving         |

    Scenario: Car - Control test without durations, osrm uses movable bridge speed to calculate duration
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | bridge  |
            | abc   | primary |         |
            | cde   |         | movable |
            | efg   | primary |         |

        When I route I should get
            | from | to | route           | modes                           | speed   | time     |
            | a    | g  | abc,cde,efg,efg | driving,driving,driving,driving | 12 km/h | 173s +-1 |
            | b    | f  | abc,cde,efg,efg | driving,driving,driving,driving | 9 km/h  | 162s +-1 |
            | c    | e  | cde,cde         | driving,driving                 | 5 km/h  | 146s +-1 |
            | e    | c  | cde,cde         | driving,driving                 | 5 km/h  | 149s +-1 |

    Scenario: Car - Properly handle durations
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | bridge  | duration |
            | abc   | primary |         |          |
            | cde   |         | movable | 00:05:00 |
            | efg   | primary |         |          |

        When I route I should get
            | from | to | route           | modes                           | speed  |
            | a    | g  | abc,cde,efg,efg | driving,driving,driving,driving | 7 km/h |
            | b    | f  | abc,cde,efg,efg | driving,driving,driving,driving | 5 km/h |
            | c    | e  | cde,cde         | driving,driving                 | 2 km/h |
            | e    | c  | cde,cde         | driving,driving                 | 2 km/h |
