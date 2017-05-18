@routing @speed @traffic
Feature: Traffic - speeds edge cases
    Scenario: Weighting based on speed file weights that cause segment weight overflows
        Given the node map
        """
        a-----b
        """
        And the ways
          | nodes | highway |
          | ab    | primary |
        And the profile file "testbot" initialized with
        """
        profile.properties.traffic_signal_penalty = 0
        profile.properties.u_turn_penalty = 0
        profile.properties.weight_precision = 2
        """
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1,2,1,0.001
        2,1,1,0.001
        """
        And the query options
          | annotations | datasources |

        When I route I should get
          | from | to | route       | speed   | weights                  | a:datasources |
          | a    | b  | ab,ab       | 1 km/h  | 41943.02,0               | 1            |
