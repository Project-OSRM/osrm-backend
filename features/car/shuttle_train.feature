@routing @car @shuttle_train
Feature: Car - Handle ferryshuttle train routes

    Background:
        Given the profile "car"

    Scenario: Car - Use a ferry route
        Given the node map
            """
            a b c
                d
                e f g h
            """

        And the ways
            | nodes | highway | route         | bicycle |
            | abc   | primary |               |         |
            | cde   |         | shuttle_train | yes     |
            | ef    | primary |               |         |
            | fg    |         | ferry_man     |         |
            | gh    | primary |               | no      |

        When I route I should get
            | from | to | route         |
            | a    | f  | abc,cde,ef,ef |
            | b    | f  | abc,cde,ef,ef |
            | e    | c  | cde,cde       |
            | e    | b  | cde,abc,abc   |
            | e    | a  | cde,abc,abc   |
            | c    | e  | cde,cde       |
            | c    | f  | cde,ef,ef     |
            | f    | g  |               |
            | g    | h  | gh,gh         |
