@testbot
Feature: Weight tests
    @traffic @speed
    Scenario: Step weights -- segment_function with speed and turn updates
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.properties.traffic_signal_penalty = 0
          profile.properties.u_turn_penalty = 0
          profile.properties.weight_name = 'steps'
          return profile
        end

        functions.process_way = function(profile, way, result)
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
          result.weight = 42
          result.duration = 3
        end

        functions.process_segment = function(profile, segment)
          segment.weight = 10
          segment.duration = 11
        end

        return functions
        """

        And the node map
            """
            a---b---c---d
                    .
                    e
            """
        And the ways
            | nodes |
            | abcd  |
            | ce    |
        And the parquet speed file
            """
            1,2,36.999,42
            2,1,36,42
            """
        And the parquet turn penalty file
            """
            2,3,5,25.5,16.7
            """
        And the contract extra arguments "--speed-and-turn-penalty-format parquet --segment-speed-file {speeds_file} --turn-penalty-file {penalties_file}"
        And the customize extra arguments "--speed-and-turn-penalty-format parquet --segment-speed-file {speeds_file} --turn-penalty-file {penalties_file}"

        When I route I should get
            | waypoints | route | distance | weights   | times        |
            | a,d       | ,     | 60m      | 20.5,0    | 23.9s,0s       |
            | a,e       | ,,    | 60m      | 27.2,10,0 | 38.4s,11s,0s |
            | d,e       | ,,    | 40m      | 10,10,0   | 11s,11s,0s   |

    