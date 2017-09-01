@routing @bicycle @ferry
Feature: Bike - Handle ferry routes

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Ferry route
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
            | from | to | route           |
            | a    | g  | abc,cde,efg,efg |
            | c    | e  | cde,cde         |
            | c    | g  | cde,efg,efg     |

    Scenario: Bike - Ferry route, access denied
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | route | bicycle |
            | abc   | primary |       |         |
            | cde   |         | ferry | no      |
            | efg   | primary |       |         |

        When I route I should get
            | from | to | route |
            | a    | g  |       |  
            | c    | e  |       |
            | c    | g  |       |

    @todo
    Scenario: Bike - Ferry route should be unattractive, even when fast
        Given the node map
            """
            c---------------d
            |               |
            b-----a ~ f-----e
            """

        And the ways
            | nodes  | highway | route | bicycle | duration |
            | abcdef | primary |       |         |          |
            | af     |         | ferry | yes     | 00:01    |

        When I route I should get
            | from | to | route         | time |
            | a    | f  | abcdef,abcdef | 432s |

    Scenario: Bike - Ferry duration, single node
        Given the node map
            """
            b c
              e
              g
              i
            """

        And the ways
            | nodes | highway | route | bicycle | duration |
            | bc    |         | ferry | yes     | 0:01     |
            | be    |         | ferry | yes     | 0:10     |
            | bg    |         | ferry | yes     | 1:00     |
            | bi    |         | ferry | yes     | 10:00    |

        When I route I should get
            | from | to | route | time     |
            | b    | c  | bc,bc | 67.1s    |
            | b    | e  | be,be | 613.8s   |
            | b    | g  | bg,bg | 3600s    |
            | b    | i  | bi,bi | 36030.6s |

    Scenario: Bike - Ferry duration, multiple nodes
        Given the node map
            """
            x a b c d y
            """

        And the ways
            | nodes | highway | route | bicycle | duration |
            | xa    | primary |       |         |          |
            | yd    | primary |       |         |          |
            | abcd  |         | ferry | yes     | 1:00     |

        When I route I should get
            | from | to | route     | time  |
            | a    | d  | abcd,abcd | 3600s |
            | d    | a  | abcd,abcd | 3600s |
