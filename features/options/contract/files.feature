@prepare @options @files
Feature: osrm-contract command line options: files
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

    Scenario: osrm-contract - Passing base file
        When I run "osrm-contract {processed_file}"
        Then it should exit successfully

    Scenario: osrm-contract - Missing input file
        When I try to run "osrm-contract over-the-rainbow.osrm"
        And stderr should contain "over-the-rainbow.osrm"
        And stderr should contain "Missing/Broken"
        And it should exit with an error
