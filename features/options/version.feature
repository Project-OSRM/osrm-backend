@routing @options
Feature: Command line options: version

    Background:
        Given the profile "testbot"

    Scenario: Version, short
        When I run "osrm-routed -v"
        Then it should exit with code 0
        And stderr should be empty
        And stdout should contain " v0."

    Scenario: Version, long
        When I run "osrm-routed --version"
        Then it should exit with code 0
        And stderr should be empty
        And stdout should contain " v0."