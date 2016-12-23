@contract @options @edge-weight-updates-over-factor
Feature: osrm-contract command line option: edge-weight-updates-over-factor

    Background: Log edge weight updates over given factor
        Given the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | 0.05       | 0.1      |
            | c    | 0.3        | 0.1      |
        And the ways
            | nodes | highway     |
            | ab    | residential |
            | ac    | primary     |
        Given the profile "testbot.lua"
        Given the speed file
        """
        1,2,100
        1,3,100
        """
        And the data has been saved to disk

    Scenario: Logging weight with updates over factor of 2, long segment
        When I run "osrm-extract --profile {profile_file} {osm_file} --generate-edge-lookup"
        When I run "osrm-contract --edge-weight-updates-over-factor 2 --segment-speed-file {speeds_file} {processed_file}"
        And stderr should contain "weight updates"
        And stderr should contain "New speed: 100 kph"
        And I route I should get
            | from | to | route    | speed     |
            | a    | b  | ab,ab    | 100 km/h  |
            | a    | c  | ac,ac    | 100 km/h  |
