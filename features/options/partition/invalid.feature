@partition @options @invalid
Feature: osrm-partition command line options: invalid options

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

    Scenario: osrm-partition - Non-existing option
        When I try to run "osrm-partition --fly-me-to-the-moon"
        Then stdout should be empty
        And stderr should contain "option"
        And stderr should contain "fly-me-to-the-moon"
        And it should exit with an error

    Scenario: osrm-partition - Check invalid values
        When I try to run "osrm-partition --max-cell-sizes 4,6@4,16 fly-me-to-the-moon.osrm"
        Then stdout should be empty
        And stderr should contain "is invalid"
        And it should exit with an error

    Scenario: osrm-partition - Check non-descending order
        When I try to run "osrm-partition --max-cell-sizes 4,64,16 fly-me-to-the-moon.osrm"
        Then stdout should be empty
        And stderr should contain "must be sorted in non-descending order"
        And it should exit with an error
