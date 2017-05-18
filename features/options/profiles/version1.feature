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
            print(node, result)
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
            print(node, result)
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

    Scenario: Weighting based on raster sources
        Given the profile file
        """
        api_version = 1

        properties.force_split_edges = true

        function source_function()
          local path = os.getenv('OSRM_RASTER_SOURCE')
          if not path then
            path = 'rastersource.asc'
          end
          raster_source = sources:load(
            path,
            0,    -- lon_min
            0.1,  -- lon_max
            0,    -- lat_min
            0.1,  -- lat_max
            5,    -- nrows
            4     -- ncols
          )
        end

        function way_function (way, result)
          result.name = way:get_value_by_key('name')
          result.forward_mode = mode.cycling
          result.backward_mode = mode.cycling
          result.forward_speed = 15
          result.backward_speed = 15
        end

        function segment_function (segment)
          local sourceData = sources:query(raster_source, segment.source.lon, segment.source.lat)
          local targetData = sources:query(raster_source, segment.target.lon, segment.target.lat)
          io.write('evaluating segment: ' .. sourceData.datum .. ' ' .. targetData.datum .. '\n')
          local invalid = sourceData.invalid_data()
          local scaled_weight = segment.weight
          local scaled_duration = segment.duration

          if sourceData.datum ~= invalid and targetData.datum ~= invalid then
            local slope = (targetData.datum - sourceData.datum) / segment.distance
            scaled_weight = scaled_weight / (1.0 - (slope * 5.0))
            scaled_duration = scaled_duration / (1.0 - (slope * 5.0))
            io.write('   slope: ' .. slope .. '\n')
            io.write('   was weight: ' .. segment.weight .. '\n')
            io.write('   new weight: ' .. scaled_weight .. '\n')
            io.write('   was duration: ' .. segment.duration .. '\n')
            io.write('   new duration: ' .. scaled_duration .. '\n')
          end

          segment.weight = scaled_weight
          segment.duration = scaled_duration
        end
        """
        And the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | 0.05       | 0.1      |
            | c    | 0.0        | 0.1      |
            | d    | 0.05       | 0.03     |
            | e    | 0.05       | 0.066    |
            | f    | 0.075      | 0.066    |
        And the ways
            | nodes | highway |
            | ab    | primary |
            | ad    | primary |
            | bc    | primary |
            | dc    | primary |
            | de    | primary |
            | eb    | primary |
            | df    | primary |
            | fb    | primary |
        And the raster source
            """
            0  0  0   0
            0  0  0   250
            0  0  250 500
            0  0  0   250
            0  0  0   0
            """
        And the data has been saved to disk

        When I route I should get
            | from | to | route    | speed   |
            | a    | b  | ab,ab    | 8 km/h  |
            | b    | a  | ab,ab    | 22 km/h |
            | a    | c  | ab,bc,bc | 12 km/h |
            | b    | c  | bc,bc    | 22 km/h |
            | a    | d  | ad,ad    | 15 km/h |
            | d    | c  | dc,dc    | 15 km/h |
            | d    | e  | de,de    | 10 km/h |
            | e    | b  | eb,eb    | 10 km/h |
            | d    | f  | df,df    | 15 km/h |
            | f    | b  | fb,fb    | 7 km/h  |
            | d    | b  | de,eb,eb | 10 km/h |
