@prepare @options @invalid
Feature: osrm-contract command line options: invalid options

    Background:
        Given the profile "testbot"

    Scenario: osrm-contract - Non-existing option
        When I try to run "osrm-contract --fly-me-to-the-moon"
        Then stdout should be empty
        And stderr should contain "option"
        And stderr should contain "fly-me-to-the-moon"
        And it should exit with an error
