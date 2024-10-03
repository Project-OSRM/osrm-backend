@routing @countryfoot @ferry
Feature: Countryfoot - Handle ferry routes

    Background:
        Given the profile "countryfoot"

    Scenario: Countryfoot - Ferry route
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway | route | foot |
            | abc   | primary |       |      |
            | cde   |         | ferry | yes  |
            | efg   | primary |       |      |

        When I route I should get
            | from | to | route           | modes                         |
            | a    | g  | abc,cde,efg,efg | walking,ferry,walking,walking |
            | e    | a  | cde,abc,abc     | ferry,walking,walking         |

    Scenario: Countryfoot - Ferry duration, single node
        Given the node map
            """
            a b c d
                e f
                g h
                i j
            """

        And the ways
            | nodes | highway | route | foot | duration |
            | ab    | primary |       |      |          |
            | cd    | primary |       |      |          |
            | ef    | primary |       |      |          |
            | gh    | primary |       |      |          |
            | ij    | primary |       |      |          |
            | bc    |         | ferry | yes  | 0:01     |
            | be    |         | ferry | yes  | 0:10     |
            | bg    |         | ferry | yes  | 1:00     |
            | bi    |         | ferry | yes  | 10:00    |

    Scenario: Countryfoot - Ferry duration, multiple nodes
        Given the node map
            """
            x         y
              a b c d
            """

        And the ways
            | nodes | highway | route | foot | duration |
            | xa    | primary |       |      |          |
            | yd    | primary |       |      |          |
            | abcd  |         | ferry | yes  | 1:00     |

        When I route I should get
            | from | to | route     | time  |
            | a    | d  | abcd,abcd | 3600s |
            | d    | a  | abcd,abcd | 3600s |
