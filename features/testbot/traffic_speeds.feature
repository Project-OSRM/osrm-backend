@routing @speed @traffic
Feature: Traffic - speeds

    Background: Use specific speeds
        #       __ a
        #      /  /
        #c----b  / g
        # \   |\/
        #  \  e/\_.f
        #   \ |  /
        #     d./
        Given the node locations
          | node |   lat |   lon | id |
          | a    |   0.1 |   0.1 | 1  |
          | b    |  0.05 |   0.1 | 2  |
          | c    |   0.0 |   0.1 | 3  |
          | d    |  0.05 |  0.03 | 4  |
          | e    |  0.05 | 0.066 | 5  |
          | f    | 0.075 | 0.066 | 6  |
          | g    | 0.075 |   0.1 | 7  |
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
        And the profile "testbot"


    Scenario: Weighting based on speed file
        Given the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1,2,0,0
        2,1,0,0
        2,3,27,7.5
        3,2,27,7.5
        1,4,27,7.5
        4,1,27,7.5
        """
        And the query options
          | annotations | datasources |

        When I route I should get
          | from | to | route       | speed   | weights              | a:datasources |
          | a    | b  | ad,de,eb,eb | 30 km/h | 1275.7,400.4,378.2,0 | 1:0:0         |
          | a    | c  | ad,dc,dc    | 31 km/h | 1275.7,956.8,0       | 1:0           |
          | b    | c  | bc,bc       | 27 km/h | 741.5,0              | 1             |
          | a    | d  | ad,ad       | 27 km/h | 1275.7,0             | 1             |
          | d    | c  | dc,dc       | 36 km/h | 956.8,0              | 0             |
          | g    | b  | fb,fb       | 36 km/h | 164.7,0              | 0             |
          | a    | g  | ad,df,fb,fb | 30 km/h | 1295.7,487.5,304.7,0 | 1:0:0         |


    Scenario: Weighting based on speed file weights, ETA based on file durations
        Given the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1,2,1,0.2777777
        2,1,1
        2,3,27
        3,2,27,7.5
        1,4,27,7.5
        4,1,27,7.5
        """
        And the query options
          | annotations | datasources |

        When I route I should get
          | from | to | route       | speed   | weights              | a:datasources |
          | a    | b  | ad,de,eb,eb | 30 km/h | 1275.7,400.4,378.2,0 | 1:0:0         |
          | a    | c  | ad,dc,dc    | 31 km/h | 1275.7,956.8,0       | 1:0           |
          | b    | c  | bc,bc       | 27 km/h | 741.5,0              | 1             |
          | a    | d  | ad,ad       | 27 km/h | 1275.7,0             | 1             |
          | d    | c  | dc,dc       | 36 km/h | 956.8,0              | 0             |
          | g    | b  | ab,ab       | 1 km/h  | 10010.4,0            | 1             |
          | a    | g  | ab,ab       | 1 km/h  | 10010.3,0            | 1             |


    Scenario: Weighting based on speed file weights, ETA based on file durations
        Given the profile file "testbot" initialized with
        """
        profile.properties.traffic_signal_penalty = 0
        profile.properties.u_turn_penalty = 0
        profile.properties.weight_precision = 2
        """
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1,2,1,0.27777777
        2,1,1,0.27777777
        2,3,27,7.5
        3,2,27
        1,4,1
        4,1,1
        """
        And the query options
          | annotations | datasources |

        When I route I should get
          | from | to | route       | speed   | weights                  | a:datasources |
          | a    | b  | ab,ab       | 1 km/h  | 20020.73,0               | 1             |
          | a    | c  | ab,bc,bc    | 2 km/h  | 20020.73,741.51,0        | 1:1           |
          | b    | c  | bc,bc       | 27 km/h | 741.51,0                 | 1             |
          | a    | d  | ab,eb,de,de | 2 km/h  | 20020.73,378.17,400.41,0 | 1:0:0         |
          | d    | c  | dc,dc       | 36 km/h | 956.8,0                  | 0             |
          | g    | b  | ab,ab       | 1 km/h  | 10010.37,0               | 1             |
          | a    | g  | ab,ab       | 1 km/h  | 10010.36,0               | 1             |
          | g    | a  | ab,ab       | 1 km/h  | 10010.36,0               | 1             |


    Scenario: Speeds that isolate a single node (a)
        Given the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the node locations
          | node | lat        | lon      |
          | h    | 2.075      | 19.1     |
        And the speed file
        """
        1,2,0
        2,1,0
        2,3,27,7.5
        3,2,27,7.5
        1,4,0,0
        4,1,0,0
        """
        And the query options
          | annotations | true |

        When I route I should get
          | from | to | route    | speed   | weights       | a:datasources | a:speed | a:nodes|
          | a    | b  | fb,fb    | 36 km/h | 329.4,0       | 0             | 10      | 6:2    |
          | a    | c  | fb,bc,bc | 30 km/h | 329.4,741.5,0 | 0:1           | 10:7.5  | 6:2:3  |
          | b    | c  | bc,bc    | 27 km/h | 741.5,0       | 1             | 7.5     | 2:3    |
          | a    | d  | fb,df,df | 36 km/h | 140,487.5,0   | 0:0           | 10:10   | 2:6:4  |
          | d    | c  | dc,dc    | 36 km/h | 956.8,0       | 0             | 10      | 4:3    |
          | g    | b  | fb,fb    | 36 km/h | 164.7,0       | 0             | 10      | 6:2    |
          | a    | g  | fb,fb    | 36 km/h | 164.7,0       | 0             | 10      | 6:2    |


    Scenario: Verify that negative values cause an error, they're not valid at all
        Given the speed file
        """
        1,2,-10
        2,1,-20
        2,3,27
        3,2,27
        1,4,-3
        4,1,-5
        """
        And the data has been extracted
        When I try to run "osrm-contract --segment-speed-file {speeds_file} {processed_file}"
        And stderr should contain "malformed"
        And it should exit with an error

    Scenario: Check with an empty speed file
        Given the speed file
        """
        """
        And the data has been extracted
        When I try to run "osrm-contract --segment-speed-file {speeds_file} {processed_file}"
        And it should exit successfully
