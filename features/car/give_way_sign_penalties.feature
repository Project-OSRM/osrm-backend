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

        # TODO: give way signs with no direction has no any impact on routing at the moment
        When I route I should get
            | from | to | time   | # |
            | 1    | 2  |  11.1s | no turn with no give way |
            | 3    | 4  |  11.1s | no turn with give way    |
            | g    | j  |  18.7s | turn with no give way    |
            | k    | n  |  18.7s | turn with give way       |


    Scenario: Car - Give way direction
        Given the node map
            """
            a-1-b-2-c

            d-3-e-4-f

            g-5-h-6-i

            j-7-k-8-l

            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | def   | primary |
            | ghi   | primary |
            | jkl   | primary |

        And the nodes
            | node | highway  | direction                 |
            | e    | give_way |                           |
            | h    | give_way | forward                   |
            | k    | give_way | backward                  |
        When I route I should get
            | from | to | time   | weight | #                        |
            | 1    | 2  |  11.1s | 11.1   | no turn with no give way |
            | 2    | 1  |  11.1s | 11.1   | no turn with no give way |
            | 3    | 4  |  11.1s | 11.1   | no turn with give way    |
            | 4    | 3  |  11.1s | 11.1   | no turn with give way    |
            | 5    | 6  |  12.6s | 12.6   | no turn with give way    |
            | 6    | 5  |  11.1s | 11.1   | no turn with no give way |
            | 7    | 8  |  11.1s | 11.1   | no turn with no give way |
            | 8    | 7  |  12.6s | 12.6   | no turn with give way    |
