@routing @bicycle @bridge
Feature: Bicycle - Handle movable bridge

    Background:
        Given the profile "bicycle"

    Scenario: Bicycle - Use a ferry route
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
            | from | to | route       | modes                          |
            | a    | g  | abc,cde,efg | cycling,movable bridge,cycling |
            | b    | f  | abc,cde,efg | cycling,movable bridge,cycling |
            | e    | c  | cde         | movable bridge                 |
            | e    | b  | cde,abc     | movable bridge,cycling         |
            | e    | a  | cde,abc     | movable bridge,cycling         |
            | c    | e  | cde         | movable bridge                 |
            | c    | f  | cde,efg     | movable bridge,cycling         |
            | c    | g  | cde,efg     | movable bridge,cycling         |

    Scenario: Bicycle - Properly handle durations
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
            | from | to | route       | modes                          | speed   |
            | a    | g  | abc,cde,efg | cycling,movable bridge,cycling | 5 km/h |
            | b    | f  | abc,cde,efg | cycling,movable bridge,cycling | 4 km/h |
            | c    | e  | cde         | movable bridge                 | 2 km/h |
            | e    | c  | cde         | movable bridge                 | 2 km/h |
