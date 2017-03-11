@partition @options @files
Feature: osrm-partition command line options: files
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

    Scenario: osrm-partition - Passing base file
        When I run "osrm-partition {processed_file}"
        Then it should exit successfully

    Scenario: osrm-partition - Missing input file
        When I try to run "osrm-partition over-the-rainbow.osrm"
        And stderr should contain "over-the-rainbow.osrm"
        And stderr should contain "not found"
        And it should exit with an error
