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
        Given the profile "testbot"
        Given the speed file
        """
        1,2,100
        1,3,100
        """
        And the data has been saved to disk

    Scenario: Logging weight with updates over factor of 2, long segment
        When I run "osrm-extract --profile {profile_file} {osm_file}"
        And the data has been partitioned
        When I run "osrm-contract --edge-weight-updates-over-factor 2 --segment-speed-file {speeds_file} {processed_file}"
        Then stderr should not contain "Speed values were used to update 2 segment(s)"
        And stderr should contain "Segment: 1,2"
        And stderr should contain "Segment: 1,3"
        And I route I should get
            | from | to | route    | speed     |
            | a    | b  | ab,ab    | 100 km/h  |
            | a    | c  | ac,ac    | 100 km/h  |


    Scenario: Logging using weigts as durations for non-duration profile
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.weight_name = 'steps'
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.weight = 1
          result.duration = 1
        end

        return functions
        """
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        When I run "osrm-contract --edge-weight-updates-over-factor 2 --segment-speed-file {speeds_file} {processed_file}"
        Then stderr should contain "Speed values were used to update 2 segments for 'steps' profile"
