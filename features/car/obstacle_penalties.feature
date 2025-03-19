@routing @car @obstacle
Feature: Car - Handle obstacle penalties

    Background:
        Given the profile "car"

    Scenario: Car - Give-Way signs
        Given the node map
            """
            a-b-c   d-e-f   g-h-i   j-k-l   m-n-o

            """

        And the ways
            | nodes   | highway |
            | abc     | primary |
            | def     | primary |
            | ghi     | primary |
            | jkl     | primary |
            | mno     | primary |

        And the nodes
            | node | highway  | direction |
            | e    | give_way |           |
            | h    | give_way | forward   |
            | k    | give_way | backward  |
            | n    | give_way | both      |

        When I route I should get
            | from | to | time | #             |
            | a    | c  | 11s  |               |
            | c    | a  | 11s  |               |
            | d    | f  | 12s  | give-way sign |
            | f    | d  | 12s  | give-way sign |
            | g    | i  | 12s  | give-way sign |
            | i    | g  | 11s  |               |
            | j    | l  | 11s  |               |
            | l    | j  | 12s  | give-way sign |
            | m    | o  | 12s  | give-way sign |
            | o    | m  | 12s  | give-way sign |


    Scenario: Car - Stop signs
        Given the node map
            """
            a-b-c   d-e-f   g-h-i   j-k-l   m-n-o

            """

        And the ways
            | nodes   | highway |
            | abc     | primary |
            | def     | primary |
            | ghi     | primary |
            | jkl     | primary |
            | mno     | primary |

        And the nodes
            | node | highway | direction |
            | e    | stop    |           |
            | h    | stop    | forward   |
            | k    | stop    | backward  |
            | n    | stop    | both      |

        When I route I should get
            | from | to | time | #         |
            | a    | c  | 11s  |           |
            | c    | a  | 11s  |           |
            | d    | f  | 13s  | stop sign |
            | f    | d  | 13s  | stop sign |
            | g    | i  | 13s  | stop sign |
            | i    | g  | 11s  |           |
            | j    | l  | 11s  |           |
            | l    | j  | 13s  | stop sign |
            | m    | o  | 13s  | stop sign |
            | o    | m  | 13s  | stop sign |


    Scenario: Car - Stop sign on intersection node
        Given the node map
            """
              a      f      k
            b-c-d  h-g-i  l-m-n
              e      j      o

            """

        And the ways
            | nodes | highway   |
            | bcd   | primary   |
            | ace   | secondary |
            | hgi   | primary   |
            | fgj   | secondary |
            | lmn   | primary   |
            | kmo   | secondary |

        And the nodes
            | node | highway | stop  |
            | g    | stop    |       |
            | m    | stop    | minor |


        When I route I should get

            # No road has stop signs
            | from | to | time    | #                    |
            | a    | b  | 14s +-1 |    turn              |
            | a    | e  | 13s +-1 | no turn              |
            | a    | d  | 17s +-1 |    turn              |
            | e    | d  | 14s +-1 |    turn              |
            | e    | a  | 13s +-1 | no turn              |
            | e    | b  | 17s +-1 |    turn              |
            | d    | a  | 14s +-1 |    turn              |
            | d    | b  | 11s +-1 | no turn              |
            | d    | e  | 17s +-1 |    turn              |
            | b    | e  | 14s +-1 |    turn              |
            | b    | d  | 11s +-1 | no turn              |
            | b    | a  | 17s +-1 |    turn              |

            # All roads have stop signs - 2s penalty
            | f    | h  | 16s +-1 |    turn with stop    |
            | f    | j  | 15s +-1 | no turn with stop    |
            | f    | i  | 19s +-1 |    turn with stop    |
            | j    | i  | 16s +-1 |    turn with stop    |
            | j    | f  | 15s +-1 | no turn with stop    |
            | j    | h  | 19s +-1 |    turn with stop    |
            | i    | f  | 16s +-1 |    turn with stop    |
            | i    | h  | 13s +-1 | no turn with stop    |
            | i    | j  | 19s +-1 |    turn with stop    |
            | h    | j  | 16s +-1 |    turn with stop    |
            | h    | i  | 13s +-1 | no turn with stop    |
            | h    | f  | 19s +-1 |    turn with stop    |

            # Minor roads have stop signs - 2s penalty
            | k    | l  | 16s +-1 |    turn with minor stop |
            | k    | o  | 15s +-1 | no turn with minor stop |
            | k    | n  | 19s +-1 |    turn with minor stop |
            | o    | n  | 16s +-1 |    turn with minor stop |
            | o    | k  | 15s +-1 | no turn with minor stop |
            | o    | l  | 19s +-1 |    turn with minor stop |
            | n    | k  | 14s +-1 |    turn                 |
            | n    | l  | 11s +-1 | no turn                 |
            | n    | o  | 17s +-1 |    turn                 |
            | l    | o  | 14s +-1 |    turn                 |
            | l    | n  | 11s +-1 | no turn                 |
            | l    | k  | 17s +-1 |    turn                 |


    Scenario: Car - Infer stop sign direction
        Given a grid size of 5 meters

        Given the node map
            """
            a---------sb

            c---------td

            e---------uf

            g---------vh

            """

        And the ways
            | nodes | highway   |
            | asb   | primary   |
            | ctd   | primary   |
            | euf   | primary   |
            | gvh   | primary   |

        And the nodes
            | node | highway | direction |
            | s    | stop    |           |
            | t    | stop    | forward   |
            | u    | stop    | backward  |
            | v    | stop    | both      |

        When I route I should get
            | from | to | time | #    |
            | a    | b  | 3.5s | stop |
            | b    | a  | 1.5s |      |
            | c    | d  | 3.5s | stop |
            | d    | c  | 1.5s |      |
            | e    | f  | 1.5s |      |
            | f    | e  | 3.5s | stop |
            | g    | h  | 3.5s | stop |
            | h    | g  | 3.5s | stop |


    Scenario: Car - Infer stop sign direction 2
        Given a grid size of 5 meters

        Given the node map
            """

                       d
                       |
            a---------sbt---------c
                       |
                       e

            """

        And the ways
            | nodes | highway |
            | asbtc | primary |
            | dbe   | primary |

        And the nodes
            | node | highway |
            | s    | stop    |
            | t    | stop    |

        When I route I should get
            | from | to | time    | #    |
            | a    | d  | 9s +- 1 | stop |
            | a    | c  | 5s +- 1 | stop |
            | a    | e  | 6s +- 1 | stop |
            | c    | e  | 9s +- 1 | stop |
            | c    | a  | 5s +- 1 | stop |
            | c    | d  | 6s +- 1 | stop |
            | d    | c  | 7s +- 1 |      |
            | d    | e  | 1s +- 1 |      |
            | d    | a  | 4s +- 1 |      |
            | e    | a  | 7s +- 1 |      |
            | e    | d  | 1s +- 1 |      |
            | e    | c  | 4s +- 1 |      |
