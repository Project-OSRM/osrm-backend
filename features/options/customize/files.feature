@customize @options @files
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
        And the data has been partitioned

    Scenario: osrm-customize - Passing base file
        When I run "osrm-customize {processed_file}"
        Then it should exit successfully

    Scenario: osrm-customize - Missing input file
        When I try to run "osrm-customize over-the-rainbow.osrm"
        And stderr should contain "over-the-rainbow.osrm"
        And stderr should contain "not found"
        And it should exit with an error
