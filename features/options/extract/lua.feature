@extract
Feature: osrm-extract lua ways:get_nodes()

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
        And the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "node id 1"
        And stdout should contain "node id 2"


    Scenario: osrm-extract location-dependent data
        Given the profile file
        """
        functions = require('testbot')

        function way_function(profile, way, result, relations)
           for _, key in ipairs({'answer', 'boolean', 'object', 'array'}) do
             print (key .. ' ' .. tostring(way:get_location_tag(key)))
           end
           result.forward_mode = mode.driving
           result.forward_speed = 1
        end

        functions.process_way = way_function
        return functions
        """
        And the node map
            """
            a b
            """
        And the ways with locations
            | nodes |
            | ab    |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson"
        Then it should exit successfully
        And stdout should contain "answer 42"
        And stdout should contain "boolean true"
        And stdout should contain "array nil"
        And stdout should contain "object nil"


    Scenario: osrm-extract location-dependent data with multi-polygons
        Given the profile file
        """
        functions = require('testbot')

        function way_function(profile, way, result, relations)
           print('ISO3166-1 ' .. (way:get_location_tag('ISO3166-1') or 'none'))
           result.forward_mode = mode.driving
           result.forward_speed = 1
        end

        functions.process_way = way_function
        return functions
        """
        And the node locations
            | node |        lat |         lon | id |
            | a    | 22.4903670 | 113.9455227 |  1 |
            | b    | 22.4901701 | 113.9455899 |  2 |
            | c    | 22.4901852 | 113.9458608 |  3 |
            | d    | 22.4904033 | 113.9456999 |  4 |
        And the ways with locations
            | nodes | #              |
            | ab    | Hong Kong      |
            | cd    | China Mainland |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson --location-dependent-data test/data/regions/hong-kong.geojson"
        Then it should exit successfully
        And stdout should contain "1 GeoJSON polygon"
        And stdout should contain "2 GeoJSON polygons"
        And stdout should contain "ISO3166-1 HK"
        And stdout should contain "ISO3166-1 none"

    Scenario: osrm-extract location-dependent data via locations cache
        Given the profile file
        """
        functions = require('testbot')

        function way_function(profile, way, result, relations)
           print ('answer ' .. tostring(way:get_location_tag('answer')))
           result.forward_mode = mode.driving
           result.forward_speed = 1
        end

        functions.process_way = way_function
        return functions
        """
        And the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson"
        Then it should exit successfully
        And stdout should contain "answer 42"
