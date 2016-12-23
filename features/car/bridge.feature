@routing @car @bridge
Feature: Car - Handle driving

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
            | a    | g  | abc,cde,efg,efg | driving,driving,driving,driving | 6 km/h |
            | b    | f  | abc,cde,efg,efg | driving,driving,driving,driving | 4 km/h |
            | c    | e  | cde,cde         | driving,driving                 | 2 km/h |
            | e    | c  | cde,cde         | driving,driving                 | 2 km/h |
