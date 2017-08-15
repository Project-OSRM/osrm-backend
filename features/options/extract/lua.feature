@extract
Feature: osrm-extract lua ways:get_nodes()

    Background:
        Given the node map
            """
            a b
            """

    Scenario: osrm-extract - Passing base file
        Given the profile file
        """
        functions = require('testbot')

        function way_function(profile, way, result)
          for _, node in ipairs(way:get_nodes()) do
            print('node id ' .. node:id())
          end
          result.forward_mode = mode.driving
          result.forward_speed = 1
        end

        functions.process_way = way_function
        return functions
        """
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "node id 1"
        And stdout should contain "node id 2"


    Scenario: osrm-extract location-dependent data without add-locations-to-ways preprocessing
        Given the profile "testbot"
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

        When I try to run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson"
        Then it should exit with an error
        And stderr should contain "invalid location"


    Scenario: osrm-extract location-dependent data
        Given the profile file
        """
        functions = require('testbot')

        function way_function(profile, way, result, location_data)
           assert(location_data)
           for k, v in pairs(location_data) do print (k .. ' ' .. tostring(v)) end
           result.forward_mode = mode.driving
           result.forward_speed = 1
        end

        functions.process_way = way_function
        return functions
        """
        And the ways with locations
            | nodes |
            | ab    |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson"
        Then it should exit successfully
        And stdout should contain "answer 42"
        And stdout should not contain "array"
