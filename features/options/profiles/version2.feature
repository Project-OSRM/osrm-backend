Feature: Profile API version 2

    Background:
        Given a grid size of 100 meters

    Scenario: Basic profile function calls and property values
        Given the profile file
            """
            api_version = 2

            Set = require('lib/set')
            Sequence = require('lib/sequence')
            Handlers = require("lib/way_handlers")
            find_access_tag = require("lib/access").find_access_tag
            limit = require("lib/maxspeed").limit


            function setup()
              return {
                properties = {
                  max_speed_for_map_matching      = 180/3.6,
                  use_turn_restrictions           = true,
                  continue_straight_at_waypoint   = true,
                  weight_name                     = 'test_version2',
                  weight_precision                = 2
                }
              }
            end

            function process_node(profile, node, result)
              print ('process_node ' .. node:id())
            end

            function process_way(profile, way, result)
              result.name = way:get_value_by_key('name')
              result.weight = 10
              result.forward_mode = mode.driving
              result.backward_mode = mode.driving
              result.forward_speed = 36
              result.backward_speed = 36
              print ('process_way ' .. way:id() .. ' ' .. result.name)
            end

            function process_turn (profile, turn)
              print('process_turn', turn.angle, turn.turn_type, turn.direction_modifier, turn.has_traffic_light)
              turn.weight = turn.angle == 0 and 0 or 4.2
              turn.duration = turn.weight
            end

            function process_segment (profile, segment)
                print ('process_segment ' .. segment.source.lon .. ' ' .. segment.source.lat)
            end

            return {
              setup = setup,
              process_node = process_node,
              process_way = process_way,
              process_segment = process_segment,
              process_turn = process_turn
            }
            """
        And the node map
            """
               a
              bcd
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
        And stdout should contain "process_node"
        And stdout should contain "process_way"
        And stdout should contain "process_turn"
        And stdout should contain "process_segment"

        When I route I should get
           | from | to | route    | time  |
           | a    | b  | ac,cb,cb | 19.2s |
           | a    | d  | ac,cd,cd | 19.2s |
           | a    | e  | ac,ce    | 20s   |
