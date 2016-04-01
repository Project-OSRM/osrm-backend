@routing @car @bridge
Feature: Car - Handle movable bridge

    Background:
        Given the profile "car"

    Scenario: Car - Use a ferry route
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | bridge  | bicycle |
            | abc   | primary |         |         |
            | cde   |         | movable | yes     |
            | efg   | primary |         |         |

        When I route I should get
            | from | to | route           | modes                                  |
            | a    | g  | abc,cde,efg,efg | driving,movable bridge,driving,driving |
            | b    | f  | abc,cde,efg,efg | driving,movable bridge,driving,driving |
            | e    | c  | cde,cde         | movable bridge,movable bridge          |
            | e    | b  | cde,abc,abc     | movable bridge,driving,driving         |
            | e    | a  | cde,abc,abc     | movable bridge,driving,driving         |
            | c    | e  | cde,cde         | movable bridge,movable bridge          |
            | c    | f  | cde,efg,efg     | movable bridge,driving,driving         |
            | c    | g  | cde,efg,efg     | movable bridge,driving,driving         |

    Scenario: Car - Properly handle durations
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | bridge  | duration |
            | abc   | primary |         |          |
            | cde   |         | movable | 00:05:00 |
            | efg   | primary |         |          |

        When I route I should get
            | from | to | route           | modes                                  | speed  |
            | a    | g  | abc,cde,efg,efg | driving,movable bridge,driving,driving | 7 km/h |
            | b    | f  | abc,cde,efg,efg | driving,movable bridge,driving,driving | 5 km/h |
            | c    | e  | cde,cde         | movable bridge,movable bridge          | 2 km/h |
            | e    | c  | cde,cde         | movable bridge,movable bridge          | 2 km/h |
