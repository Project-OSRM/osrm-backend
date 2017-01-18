@prepare @options @files
Feature: osrm-contract command line options: datasources
# expansions:
# {processed_file} => path to .osrm file

    Background:
        Given the profile "testbot.lua"
        Given the extract extra arguments "--generate-edge-lookup"
        And the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the speed file
        """
        1,2,50
        2,1,50
        2,3,50
        3,2,50
        1,4,50
        4,1,50
        """
        And the data has been extracted

    Scenario: osrm-contract - Passing base file
        When I run "osrm-contract --segment-speed-file {speeds_file} {processed_file}"
        Then datasource names should contain "lua profile,27_osrmcontract_passing_base_file_speeds"
        And it should exit successfully
