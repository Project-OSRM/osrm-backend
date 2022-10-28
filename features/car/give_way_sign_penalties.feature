@routing @car @give_way_sign
Feature: Car - Handle give way signs

    Background:
        Given the profile "car"

    Scenario: Car - Encounters a give way sign
        Given the node map
            """
            a-1-b-2-c

            d-3-e-4-f

            g-h-i   k-l-m
              |       |
              j       n

            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | def   | primary |
            | ghi   | primary |
            | klm   | primary |
            | hj    | primary |
            | ln    | primary |

        And the nodes
            | node | highway         |
            | e    | give_way        |
            | l    | give_way        |

        When I route I should get
            | from | to | time   | # |
            | 1    | 2  |  11.1s | no turn with no stop sign |
            | 3    | 4  |  13.1s | no turn with stop sign    |
            | g    | j  |  18.7s | turn with no stop sign    |
            | k    | n  |  20.7s | turn with stop sign       |