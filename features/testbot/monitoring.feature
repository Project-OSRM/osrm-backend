@routing
Feature: Basic Prometheus monitoring

    Background:
        Given the profile "testbot"
        Given a grid size of 100 meters

    @smallest
    Scenario: Monitoring fog test
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I monitor I should get
            | key          | value |
            | workers_busy | 0     |
