@routing @graph @testbot
Feature: Geometry Compression

    Background:
        Given the profile "testbot"

    Scenario: Compressed segments have correct order
        Given the node map
            """
            a   d       h
            b       e   f
              c         g
            """

        And the ways
            | nodes  |
            | abcdef |
            | fh     |
            | fg     |

        When I route I should get
            | from | to | route         | distance | speed   |
            | b    | e  | abcdef,abcdef | 588.5m   | 36 km/h |
            | e    | b  | abcdef,abcdef | 588.5m   | 36 km/h |
