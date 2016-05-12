@prepare @options @invalid
Feature: osrm-contract command line options: invalid options

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

    Scenario: osrm-contract - Non-existing option
        When I try to run "osrm-contract --fly-me-to-the-moon"
        Then stdout should be empty
        And stderr should contain "option"
        And stderr should contain "fly-me-to-the-moon"
        And it should exit with an error

    # This tests the error messages when you try to use --segment-speed-file,
    # but osrm-extract has not been run with --generate-edge-lookup
    Scenario: osrm-contract - Someone forgot --generate-edge-lookup on osrm-extract
        When I try to run "osrm-contract --segment-speed-file /dev/null {processed_file}"
        Then stderr should contain "Error while trying to mmap"
        Then stderr should contain ".osrm.turn_penalties_index"
        And it should exit with an error
