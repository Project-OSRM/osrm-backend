@routing @bicycle @bridge
Feature: Bicycle - Handle cycling

    Background:
        Given the profile "bicycle"

    Scenario: Bicycle - Use a ferry route
        Given the node map
            """
            a b   c
                  d
                  e   f g
            """

        And the ways
            | nodes | highway | bridge  | bicycle |
            | abc   | primary |         |         |
            | cde   |         | movable | yes     |
            | efg   | primary |         |         |

        When I route I should get
            | from | to | route           | modes                           |
            | a    | g  | abc,cde,efg,efg | cycling,cycling,cycling,cycling |
            | b    | f  | abc,cde,efg,efg | cycling,cycling,cycling,cycling |
            | e    | c  | cde,cde         | cycling,cycling                 |
            | e    | b  | cde,abc,abc     | cycling,cycling,cycling         |
            | e    | a  | cde,abc,abc     | cycling,cycling,cycling         |
            | c    | e  | cde,cde         | cycling,cycling                 |
            | c    | f  | cde,efg,efg     | cycling,cycling,cycling         |
            | c    | g  | cde,efg,efg     | cycling,cycling,cycling         |

    Scenario: Bicycle - Properly handle durations
        Given the node map
            """
            a b   c
                  d
                  e   f g
            """

        And the ways
            | nodes | highway | bridge  | duration |
            | abc   | primary |         |          |
            | cde   |         | movable | 00:05:00 |
            | efg   | primary |         |          |

        When I route I should get
            | from | to | route           | modes                           | speed  |
            | a    | g  | abc,cde,efg,efg | cycling,cycling,cycling,cycling | 6 km/h |
            | b    | f  | abc,cde,efg,efg | cycling,cycling,cycling,cycling | 5 km/h |
            | c    | e  | cde,cde         | cycling,cycling                 | 2 km/h |
            | e    | c  | cde,cde         | cycling,cycling                 | 2 km/h |
