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
            | from | to | route           | modes                                  |
            | a    | g  | abc,cde,efg,efg | cycling,movable bridge,cycling,cycling |
            | b    | f  | abc,cde,efg,efg | cycling,movable bridge,cycling,cycling |
            | e    | c  | cde,cde         | movable bridge,movable bridge          |
            | e    | b  | cde,abc,abc     | movable bridge,cycling,cycling         |
            | e    | a  | cde,abc,abc     | movable bridge,cycling,cycling         |
            | c    | e  | cde,cde         | movable bridge,movable bridge          |
            | c    | f  | cde,efg,efg     | movable bridge,cycling,cycling         |
            | c    | g  | cde,efg,efg     | movable bridge,cycling,cycling         |

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
            | from | to | route           | modes                                  | speed  |
            | a    | g  | abc,cde,efg,efg | cycling,movable bridge,cycling,cycling | 5 km/h |
            | b    | f  | abc,cde,efg,efg | cycling,movable bridge,cycling,cycling | 4 km/h |
            | c    | e  | cde,cde         | movable bridge,movable bridge          | 2 km/h |
            | e    | c  | cde,cde         | movable bridge,movable bridge          | 2 km/h |
