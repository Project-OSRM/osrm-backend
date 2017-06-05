@routed @options @invalid
Feature: osrm-routed command line options: invalid options

    Background:
        Given the profile "testbot"

    Scenario: osrm-routed - Non-existing option
        When I try to run "osrm-routed --fly-me-to-the-moon"
        Then stdout should be empty
        And stderr should contain "unrecognised"
        And stderr should contain "fly-me-to-the-moon"
        And it should exit with an error

    Scenario: osrm-routed - Missing file
        When I try to run "osrm-routed over-the-rainbow.osrm"
        Then stderr should contain "over-the-rainbow.osrm"
        And stderr should contain "Required files are missing"
        And it should exit with an error
