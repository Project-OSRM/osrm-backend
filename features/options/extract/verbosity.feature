@extract
Feature: osrm-extract must be silent with NONE

    Background:
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

    Scenario: osrm-extract - Passing base file with verbosity NONE
        Given the profile file
        """
        functions = require('testbot')

        function way_function(profile, way, result)
          result.forward_mode = mode.driving
          result.forward_speed = 1
        end

        functions.process_way = way_function
        return functions
        """
        When I run "osrm-extract --profile {profile_file} {osm_file} --verbosity NONE"
        Then it should exit successfully
        And stdout should not contain "[info]"
        And stdout should not contain "[error]"
        And stdout should not contain "10%"
        And stderr should be empty
