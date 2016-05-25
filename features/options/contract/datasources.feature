@prepare @options @files
Feature: osrm-contract command line options: datasources
# expansions:
# {extracted_base} => path to current extracted input file
# {profile} => path to current profile script

    Background:
        Given the profile "testbot"
        Given the extract extra arguments "--generate-edge-lookup"
        And the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        And the speed file
        """
        1,2,27
        2,1,27
        2,3,27
        3,2,27
        1,4,27
        4,1,27
        """
        And the data has been extracted

    Scenario: osrm-contract - Passing base file
        When I run "osrm-contract --segment-speed-file speeds.csv {extracted_base}.osrm"
        Then stderr should be empty
        And datasource names should contain "lua profile,speeds"
        And it should exit with code 0
