@routing @speed @traffic
Feature: Traffic - speeds

    Background: Use specific speeds
        Given the node map
        """
                  2a
                /  |
              f    5
           //  \   4
          //     \g|
         d--- e --1b
          \        |
            \      |
              \    |
                \  |
                  3c
        """

        And the nodes
            | node | id |
            | a    | 1  |
            | b    | 2  |
            | c    | 3  |
            | d    | 4  |
            | e    | 5  |
            | f    | 6  |
            | g    | 7  |

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
          | from | to | route          | speed   | weights           | a:datasources |
          | 2    | 1  | ad,de,eb,eb    | 30 km/h | 89.6,25,20,0      | 1:0:0         |
          | a    | c  | ad,dc,dc       | 31 km/h | 94.3,70.7,0       | 1:0:1         |
          | b    | c  | bc,bc          | 27 km/h | 66.7,0            | 1             |
          | a    | d  | ad,ad          | 27 km/h | 94.3,0            | 1             |
          | d    | c  | dc,dc          | 36 km/h | 70.7,0            | 0:1           |
          | g    | b  | fb,fb          | 36 km/h | 10.8,0            | 0:0           |
          | a    | g  | ad,de,eb,fb,fb | 31 km/h | 94.3,25,25,10.8,0 | 1:0:0:0       |


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
          | 2    | 1  | ad,de,eb,eb | 30 km/h | 89.6,25,20,0         | 1:0:0         |
          | 2    | 3  | ad,dc,dc    | 31 km/h | 89.6,67.2,0          | 1:0           |
          | b    | c  | bc,bc       | 27 km/h | 66.7,0               | 1             |
          | a    | d  | ad,ad       | 27 km/h | 94.3,0               | 1             |
          | d    | c  | dc,dc       | 36 km/h | 70.7,0               | 0:1           |
          | 4    | 5  | ab,ab       | 1 km/h  | 360,0                | 1             |
          | 5    | 4  | ab,ab       | 1 km/h  | 360,0                | 1             |


    Scenario: Weighting based on speed file weights, ETA based on file durations
        Given the profile file "testbot" extended with
        """
        api_version = 1
        properties.traffic_signal_penalty = 0
        properties.u_turn_penalty = 0
        properties.weight_precision = 3
        """
        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"
        And the speed file
        """
        1,2,1,0.499962
        2,1,1,0.499962
        2,3,27,7.5
        3,2,27
        1,4,1
        4,1,1
        """
        And the query options
          | annotations | datasources |

        When I route I should get
          | from | to | route       | speed   | weights          | a:datasources |
          | a    | b  | ab,ab       | 1 km/h  | 1000,0           | 1             |
          | a    | c  | ab,bc,bc    | 2 km/h  | 1000,66.676,0    | 1:1           |
          | b    | c  | bc,bc       | 27 km/h | 66.676,0         | 1             |
          | a    | d  | ab,eb,de,de | 2 km/h  | 1000,25,24.989,0 | 1:0:0:1       |
          | d    | c  | dc,dc       | 36 km/h | 70.708,0         | 0:1           |
          | 4    | b  | ab,ab       | 1 km/h  | 400,0            | 1             |
          | 5    | 4  | ab,ab       | 1 km/h  | 200,0            | 1             |
          | 4    | 5  | ab,ab       | 1 km/h  | 200,0            | 1             |


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
          | from | to | route    | speed   | weights     | a:datasources |
          | a    | b  | fb,fb    | 36 km/h | 38.3,0      | 0:0           |
          | a    | c  | fb,bc,bc | 30 km/h | 38.3,66.7,0 | 0:1           |
          | b    | c  | bc,bc    | 27 km/h | 66.7,0      | 1             |
          | a    | d  | fb,df,df | 36 km/h | 0.7,39,0    | 0:0           |
          | d    | c  | dc,dc    | 36 km/h | 70.7,0      | 0:1           |
          | g    | b  | fb,fb    | 36 km/h | 10.8,0      | 0:0           |
          | a    | g  | fb,fb    | 36 km/h | 27.5,0      | 0             |


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
