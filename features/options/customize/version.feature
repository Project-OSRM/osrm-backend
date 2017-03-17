@prepare @options @version
Feature: osrm-customize command line options: version

    Background:
        Given the profile "testbot"

    Scenario: osrm-customize - Version, short
        When I run "osrm-customize --v"
        Then stderr should be empty
        And stdout should contain 1 line
        And stdout should contain /(v\d{1,2}\.\d{1,2}\.\d{1,2}|\w*-\d+-\w+)/
        And it should exit successfully

    Scenario: osrm-customize - Version, long
        When I run "osrm-customize --version"
        Then stderr should be empty
        And stdout should contain 1 line
        And stdout should contain /(v\d{1,2}\.\d{1,2}\.\d{1,2}|\w*-\d+-\w+)/
        And it should exit successfully
