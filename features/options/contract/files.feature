@prepare @options @files
Feature: osrm-contract command line options: files
    Background:
        Given the profile "testbot"
        And the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        And the data has been extracted

    Scenario: osrm-contract - Passing base file
        When I run "osrm-contract {processed_file}"
        Then stderr should be empty
        And it should exit with code 0

    Scenario: osrm-contract - Missing input file
        When I run "osrm-contract over-the-rainbow.osrm"
        And stderr should contain "over-the-rainbow.osrm"
        And stderr should contain "not found"
        And it should exit with code 1
