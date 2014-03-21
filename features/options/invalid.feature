@routing @options
Feature: Command line options: invalid options

    Background:
        Given the profile "testbot"

    Scenario: Non-existing option
        When I run "osrm-routed --fly-me-to-the-moon"
        Then it should exit with code 255
        Then stdout should be empty
        And stderr should contain "fly-me-to-the-moon"

    Scenario: Missing file
        When I run "osrm-routed over-the-rainbow.osrm"
        Then it should exit with code 255
        And stderr should contain "does not exist"