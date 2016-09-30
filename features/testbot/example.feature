@routing @testbot @example
Feature: Testbot - Walkthrough
# A complete walk-through of how this data is processed can be found at:
# https://github.com/DennisOSRM/Project-OSRM/wiki/Processing-Flow

    Background:
        Given the profile "testbot"

    Scenario: Testbot - Processing Flow
        Given the node map
            """
                  d
            a b c
                  e
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary |        |
            | cd    | primary | yes    |
            | ce    | river   |        |
            | de    | primary |        |

        When I route I should get
            | from | to | route         |
            | a    | b  | abc,abc       |
            | a    | c  | abc,abc       |
            | a    | d  | abc,cd,cd     |
            | a    | e  | abc,ce,ce     |
            | b    | a  | abc,abc       |
            | b    | c  | abc,abc       |
            | b    | d  | abc,cd,cd     |
            | b    | e  | abc,ce,ce     |
            | d    | a  | de,ce,abc,abc |
            | d    | b  | de,ce,abc,abc |
            | d    | e  | de,de         |
            | e    | a  | ce,abc,abc    |
            | e    | b  | ce,abc,abc    |
            | e    | c  | ce,ce         |
