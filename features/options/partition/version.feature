@partition @options @version
Feature: osrm-partition command line options: version

    Background:
        Given the profile "testbot"

    Scenario: osrm-partition - Version, short
        When I run "osrm-partition --v"
        Then stderr should be empty
        And stdout should contain 1 line
        And stdout should contain /(v\d{1,2}\.\d{1,2}\.\d{1,2}|\w*-\d+-\w+)/
        And it should exit successfully

    Scenario: osrm-partition - Version, long
        When I run "osrm-partition --version"
        Then stderr should be empty
        And stdout should contain 1 line
        And stdout should contain /(v\d{1,2}\.\d{1,2}\.\d{1,2}|\w*-\d+-\w+)/
        And it should exit successfully
