Feature: Profile API version 1

    Background:
        Given a grid size of 100 meters

    Scenario: Basic profile function calls and property values
        Given the profile file
           """
api_version = 1

-- set profile properties
properties.max_speed_for_map_matching      = 180/3.6
properties.use_turn_restrictions           = true
properties.continue_straight_at_waypoint   = true
properties.weight_name                     = 'test_version1'
properties.weight_precision                = 2

assert(properties.max_turn_weight == 327.67)

function node_function (node, result)
  print ('node_function ' .. node:id())
end

function way_function(way, result)
  result.name = way:get_value_by_key('name')
  result.weight = 10
  result.forward_mode = mode.driving
  result.backward_mode = mode.driving
  result.forward_speed = 36
  result.backward_speed = 36
  print ('way_function ' .. way:id() .. ' ' .. result.name)
end

function turn_function (turn)
  print('turn_function', turn.angle, turn.turn_type, turn.direction_modifier, turn.has_traffic_light)
  turn.weight = turn.angle == 0 and 0 or 4.2
  turn.duration = turn.weight
end

function segment_function (segment)
    print ('segment_function ' .. segment.source.lon .. ' ' .. segment.source.lat)
end
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
        And stdout should contain "node_function"
        And stdout should contain "way_function"
        And stdout should contain "turn_function"
        And stdout should contain "segment_function"

        When I route I should get
           | from | to | route    | time  |
           | a    | b  | ac,cb,cb | 19.2s |
           | a    | d  | ac,cd,cd | 19.2s |
           | a    | e  | ac,ce    | 20s   |
