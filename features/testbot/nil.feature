@routing @testbot @nil
Feature: Testbot - Check assigning nil values
    Scenario: Assign nil values to all way strings
        Given the profile file "testbot" extended with
        """
        function way_function (way, result)
            result.name = "name"
            result.ref = "ref"
            result.destinations = "destinations"
            result.pronunciation = "pronunciation"
            result.turn_lanes_forward = "turn_lanes_forward"
            result.turn_lanes_backward = "turn_lanes_backward"

            result.name = nil
            result.ref = nil
            result.destinations = nil
            result.pronunciation = nil
            result.turn_lanes_forward = nil
            result.turn_lanes_backward = nil

            result.forward_speed = 10
            result.backward_speed = 10
            result.forward_mode = mode.driving
            result.backward_mode = mode.driving
        end
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
