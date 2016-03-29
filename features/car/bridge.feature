@routing @car @bridge
Feature: Car - Handle movable bridge

    Background:
        Given the profile "car"

    @mokob @2155
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
            | from | to | route       | modes |
            | a    | g  | abc,cde,efg | driving,movable bridge,driving |
            | b    | f  | abc,cde,efg | driving,movable bridge,driving |
            | e    | c  | cde         | movable bridge                 |
            | e    | b  | cde,abc     | movable bridge,driving         |
            | e    | a  | cde,abc     | movable bridge,driving         |
            | c    | e  | cde         | movable bridge                 |
            | c    | f  | cde,efg     | movable bridge,driving         |
            | c    | g  | cde,efg     | movable bridge,driving         |

    @mokob @2155
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
            | from | to | route       | modes                          | speed  |
            | a    | g  | abc,cde,efg | driving,movable bridge,driving | 7 km/h |
            | b    | f  | abc,cde,efg | driving,movable bridge,driving | 5 km/h |
            | c    | e  | cde         | movable bridge                 | 2 km/h |
            | e    | c  | cde         | movable bridge                 | 2 km/h |
