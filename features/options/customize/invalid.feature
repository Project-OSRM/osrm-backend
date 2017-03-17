@prepare @options @invalid
Feature: osrm-customize command line options: invalid options

    Background:
        Given the profile "testbot"
        And the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been extracted

    Scenario: osrm-customize - Non-existing option
        When I try to run "osrm-customize --fly-me-to-the-moon"
        Then stdout should be empty
        And stderr should contain "option"
        And stderr should contain "fly-me-to-the-moon"
        And it should exit with an error
