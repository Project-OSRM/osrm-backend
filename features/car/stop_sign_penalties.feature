@routing @car @stop_sign
Feature: Car - Handle stop signs

    Background:
        Given the profile "car"

    Scenario: Car - Encounters a stop sign
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
            | e    | stop            |
            | l    | stop            |

        # TODO: stop signs with no direction has no any impact on routing at the moment
        When I route I should get
            | from | to | time   | # |
            | 1    | 2  |  11.1s | no turn with no stop sign |
            | 3    | 4  |  11.1s | no turn with stop sign    |
            | g    | j  |  18.7s | turn with no stop sign    |
            | k    | n  |  18.7s | turn with stop sign       |

    Scenario: Car - Stop sign direction
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
            | node | highway | direction                 |
            | e    | stop    |                           |
            | h    | stop    | forward                   |
            | k    | stop    | backward                  |

        When I route I should get
            | from | to | time   | weight | #                         |
            | 1    | 2  |  11.1s | 11.1   | no turn with no stop sign |
            | 2    | 1  |  11.1s | 11.1   | no turn with no stop sign |
            | 3    | 4  |  11.1s | 11.1   | no turn with stop sign    |
            | 4    | 3  |  11.1s | 11.1   | no turn with stop sign    |
            | 5    | 6  |  13.1s | 13.1   | no turn with stop sign    |
            | 6    | 5  |  11.1s | 11.1   | no turn with no stop sign |
            | 7    | 8  |  11.1s | 11.1   | no turn with no stop sign |
            | 8    | 7  |  13.1s | 13.1   | no turn with stop sign    |
