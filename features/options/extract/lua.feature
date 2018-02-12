@extract
Feature: osrm-extract lua ways:get_nodes()

    Scenario: osrm-extract - Passing base file
        Given the profile file
        """
        functions = require('testbot')

        functions.process_way = function(profile, way, result)
          for _, node in ipairs(way:get_nodes()) do
            print('node id ' .. node:id())
          end
          result.forward_mode = mode.driving
          result.forward_speed = 1
        end

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


    Scenario: osrm-extract location-dependent data without add-locations-to-ways preprocessing and node locations cache
        Given the profile file
        """
        functions = require('testbot')

        functions.process_way = function(profile, way, result, relations)
           print(way:get_location_tag('driving_side'))
        end

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

        When I try to run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson --disable-location-cache"
        Then it should exit with an error
        And stderr should contain "invalid location"

    Scenario: osrm-extract location-dependent data
        Given the profile file
        """
        functions = require('testbot')

        functions.process_way = function(profile, way, result, relations)
           for _, key in ipairs({'answer', 'boolean', 'object', 'array'}) do
             print (key .. ' ' .. tostring(way:get_location_tag(key)))
           end
           result.forward_mode = mode.driving
           result.forward_speed = 1
        end

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

        When I run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson  --disable-location-cache"
        Then it should exit successfully
        And stdout should contain "answer 42"
        And stdout should contain "boolean true"
        And stdout should contain "array nil"
        And stdout should contain "object nil"


    Scenario: osrm-extract location-dependent data with multi-polygons
        Given the profile file
        """
        functions = require('testbot')

        functions.process_way = function(profile, way, result, relations)
           print('ISO3166-1 ' .. (way:get_location_tag('ISO3166-1') or 'none'))
           print('answer ' .. (way:get_location_tag('answer') or 'none'))
           result.forward_mode = mode.driving
           result.forward_speed = 1
        end

        return functions
        """
        And the node locations
            | node |        lat |         lon | id |
            | a    | 22.4903670 | 113.9455227 |  1 |
            | b    | 22.4901701 | 113.9455899 |  2 |
            | c    | 22.4901852 | 113.9458608 |  3 |
            | d    | 22.4904033 | 113.9456999 |  4 |
            | e    |        1.1 |           1 |  5 |
            | f    |        1.2 |           1 |  6 |
        And the ways with locations
            | nodes | #              |
            | ab    | Hong Kong      |
            | cd    | China Mainland |
            | ef    | Null Island    |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file} --location-dependent-data test/data/regions/null-island.geojson --location-dependent-data test/data/regions/hong-kong.geojson --disable-location-cache"
        Then it should exit successfully
        And stdout should not contain "1 GeoJSON polygon"
        And stdout should contain "2 GeoJSON polygons"
        And stdout should contain "ISO3166-1 HK"
        And stdout should contain "ISO3166-1 none"
        And stdout should contain "answer 42"

    Scenario: osrm-extract location-dependent data via locations cache
        Given the profile file
        """
        functions = require('testbot')

        functions.process_node = function(profile, node, result, relations)
           print ('node ' .. tostring(node:get_location_tag('answer')))
        end

        functions.process_way = function(profile, way, result, relations)
           print ('way ' .. tostring(way:get_location_tag('answer')))
           result.forward_mode = mode.driving
           result.forward_speed = 1
        end

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
        And stdout should contain "node 42"
        And stdout should contain "way 42"
