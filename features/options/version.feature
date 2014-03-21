@routing @options
Feature: Command line options: version

    Background:
        Given the profile "testbot"

    Scenario: Version, short
        When I run "osrm-routed -v"
        Then stderr should be empty
        And stdout should contain " v0."
        And it should exit with code 0

    Scenario: Version, long
        When I run "osrm-routed --version"
        Then stderr should be empty
        And stdout should contain " v0."
        And it should exit with code 0