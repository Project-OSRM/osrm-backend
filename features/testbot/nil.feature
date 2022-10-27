@routing @testbot @nil
Feature: Testbot - Check assigning empty values
    Scenario: Assign empty values to all way strings
        Given the profile file
        """
        functions = require('testbot')

        function way_function(profile, way, result)
          result.name = ""
          result.ref = ""
          result.destinations = ""
          result.exits = ""
          result.pronunciation = ""
          result.turn_lanes_forward = ""
          result.turn_lanes_backward = ""

          result.forward_speed = 10
          result.backward_speed = 10
          result.forward_mode = mode.driving
          result.backward_mode = mode.driving
        end

        functions.process_way = way_function
        return functions
        """
        Given the node map
            """
            a   b   c

                d
            """
        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bd    |

        When I route I should get
            | from | to | route |
            | a    | d  | ,,    |
            | d    | a  | ,,    |
