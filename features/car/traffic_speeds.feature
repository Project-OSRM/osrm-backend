@routing @speed @traffic
Feature: Traffic - speeds

    Background: Use specific speeds
        Given the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | .05        | 0.1      |
            | c    | 0.0        | 0.1      |
            | d    | .05        | .03      |
            | e    | .05        | .066     |
            | f    | .075       | .066     |
            | g    | .075       | 0.1      |
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
        And the speed file
        """
        1,2,27
        2,1,27
        2,3,27
        3,2,27
        1,4,27
        4,1,27
        """

    Scenario: Weighting based on speed file
        Given the profile "testbot"
        Given the extract extra arguments "--generate-edge-lookup"
        Given the contract extra arguments "--segment-speed-file speeds.csv"
        And I route I should get
            | from | to | route    | speed   |
            | a    | b  | ab,ab    | 27 km/h |
            | a    | c  | ab,bc,bc | 27 km/h |
            | b    | c  | bc,bc    | 27 km/h |
            | a    | d  | ad,ad    | 27 km/h |
            | d    | c  | dc,dc    | 36 km/h |
            | g    | b  | ab,ab    | 27 km/h |
            | a    | g  | ab,ab    | 27 km/h |

