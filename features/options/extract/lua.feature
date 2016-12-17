@extract
Feature: osrm-extract lua ways:get_nodes()

    Background:
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

    Scenario: osrm-extract - Passing base file
        Given the profile file "testbot" extended with
        """
        function way_function(way, result)
          for _, node in ipairs(way:get_nodes()) do
            print('node id ' .. node:id())
          end
          result.forward_mode = mode.driving
          result.forward_speed = 1
        end
        """
        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "node id 1"
        And stdout should contain "node id 2"
