@routing @speed @traffic
Feature: Traffic - speeds

    Background: Use specific speeds

    Scenario: Weighting based on speed file
        Given the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | 0.05       | 0.1      |
            | c    | 0.0        | 0.1      |
            | d    | 0.05       | 0.03     |
            | e    | 0.05       | 0.066    |
            | f    | 0.075      | 0.066    |
            | g    | 0.075      | 0.1      |
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
        Given the profile "testbot.lua"
        Given the extract extra arguments "--generate-edge-lookup"
        Given the contract extra arguments "--segment-speed-file {speeds_file}"
        Given the speed file
        """
        1,2,0
        2,1,0
        2,3,27
        3,2,27
        1,4,27
        4,1,27
        """
        And I route I should get
            | from | to | route          | speed   |
            | a    | b  | ad,de,eb,eb    | 30 km/h |
            | a    | c  | ad,dc,dc       | 31 km/h |
            | b    | c  | bc,bc          | 27 km/h |
            | a    | d  | ad,ad          | 27 km/h |
            | d    | c  | dc,dc          | 36 km/h |
            | g    | b  | fb,fb          | 36 km/h |
            | a    | g  | ad,df,fb,fb    | 30 km/h |


    Scenario: Speeds that isolate a single node (a)
        Given the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | 0.05       | 0.1      |
            | c    | 0.0        | 0.1      |
            | d    | 0.05       | 0.03      |
            | e    | 0.05       | 0.066     |
            | f    | 0.075      | 0.066     |
            | g    | 0.075      | 0.1      |
            | h    | 2.075      | 19.1     |
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
        Given the profile "testbot.lua"
        Given the extract extra arguments "--generate-edge-lookup"
        Given the contract extra arguments "--segment-speed-file {speeds_file}"
        Given the speed file
        """
        1,2,0
        2,1,0
        2,3,27
        3,2,27
        1,4,0
        4,1,0
        """
        And I route I should get
            | from | to | route          | speed   |
            | a    | b  | fb,fb          | 36 km/h |
            | a    | c  | fb,bc,bc       | 30 km/h |
            | b    | c  | bc,bc          | 27 km/h |
            | a    | d  | fb,df,df       | 36 km/h |
            | d    | c  | dc,dc          | 36 km/h |
            | g    | b  | fb,fb          | 36 km/h |
            | a    | g  | fb,fb          | 36 km/h |

    Scenario: Verify that negative values cause an error, they're not valid at all
        Given the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | 0.05       | 0.1      |
            | c    | 0.0        | 0.1      |
            | d    | 0.05       | 0.03     |
            | e    | 0.05       | 0.066    |
            | f    | 0.075      | 0.066    |
            | g    | 0.075      | 0.1      |
            | h    | 1.075      | 10.1     |
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
        Given the profile "testbot.lua"
        Given the extract extra arguments "--generate-edge-lookup"
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
