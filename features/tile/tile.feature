Feature: Tile service
    Background:
        Given the profile "testbot"
    Scenario: Smoke test
        Given the origin 52.5212,13.3919
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        When I request /tile/v1/testbot/tile(8800,5373,14).mvt
        Then HTTP code should be 200
