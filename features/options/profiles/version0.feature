Feature: Profile API version 0

    Scenario: Profile api version 0
        Given the profile file
        """
        api_version = 0
        -- set profile properties
        properties.u_turn_penalty                  = 20
        properties.traffic_signal_penalty          = 2
        properties.max_speed_for_map_matching      = 180/3.6
        properties.use_turn_restrictions           = true
        properties.continue_straight_at_waypoint   = true
        properties.left_hand_driving               = false
        properties.weight_name                     = 'duration'
        function node_function (node, result)
          print ('node_function ' .. node:id())
        end
        function way_function(way, result)
          result.name = way:get_value_by_key('name')
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.forward_speed = 36
          result.backward_speed = 36
          print ('way_function ' .. way:id() .. ' ' .. result.name)
        end
        function turn_function (angle)
          print('turn_function ' .. angle)
          return angle == 0 and 0 or 42
        end
        function segment_function (source, target, distance, weight)
            print ('segment_function ' .. source.lon .. ' ' .. source.lat)
        end
        """
        And the node map
           """
               a
             b c d
               e
           """
        And the ways
            | nodes  |
            | ac     |
            | cb     |
            | cd     |
            | ce     |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "node_function"
        And stdout should contain "way_function"
        And stdout should contain "turn_function"
        And stdout should contain "segment_function"

        When I route I should get
           | from | to | route    | time  |
           | a    | b  | ac,cb,cb | 24.2s |
           | a    | d  | ac,cd,cd | 24.2s |
           | a    | e  | ac,ce    | 20s   |

    Scenario: Profile version undefined, assume version 0
        Given the profile file
        """
        -- set profile properties
        properties.u_turn_penalty                  = 20
        properties.traffic_signal_penalty          = 2
        properties.max_speed_for_map_matching      = 180/3.6
        properties.use_turn_restrictions           = true
        properties.continue_straight_at_waypoint   = true
        properties.left_hand_driving               = false
        properties.weight_name                     = 'duration'
        function node_function (node, result)
          print ('node_function ' .. node:id())
        end
        function way_function(way, result)
          result.name = way:get_value_by_key('name')
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.forward_speed = 36
          result.backward_speed = 36
          print ('way_function ' .. way:id() .. ' ' .. result.name)
        end
        function turn_function (angle)
          print('turn_function ' .. angle)
          return angle == 0 and 0 or 42
        end
        function segment_function (source, target, distance, weight)
            print ('segment_function ' .. source.lon .. ' ' .. source.lat)
        end
        """
        And the node map
           """
               a
             b c d
               e
           """
        And the ways
            | nodes  |
            | ac     |
            | cb     |
            | cd     |
            | ce     |
        And the data has been saved to disk

        When I run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit successfully
        And stdout should contain "node_function"
        And stdout should contain "way_function"
        And stdout should contain "turn_function"
        And stdout should contain "segment_function"

        When I route I should get
           | from | to | route    | time  |
           | a    | b  | ac,cb,cb | 24.2s |
           | a    | d  | ac,cd,cd | 24.2s |
           | a    | e  | ac,ce    | 20s   |