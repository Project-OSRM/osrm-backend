@matrix @testbot
Feature: Basic Distance Matrix

    Background:
        Given the profile "testbot"

    Scenario: A single way with two nodes
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I request a travel time matrix I should get
            |   | a   | b   |
            | a | 0   | 100 |
            | b | 100 | 0   |
