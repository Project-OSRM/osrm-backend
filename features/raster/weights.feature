@routing @speed @raster
Feature: Raster - weights

    Background: Use specific speeds
        Given the node locations
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

    Scenario: Weighting not based on raster sources
        Given the profile "testbot"
        When I run "osrm-extract {osm_file} -p {profile_file}"
        And I run "osrm-contract {processed_file}"
        And I route I should get
            | from | to | route    | speed   |
            | a    | b  | ab,ab    | 36 km/h |
            | a    | c  | ab,bc,bc | 36 km/h |
            | b    | c  | bc,bc    | 36 km/h |
            | a    | d  | ad,ad    | 36 km/h |
            | d    | c  | dc,dc    | 36 km/h |

    Scenario: Weighting based on raster sources
        Given the profile "rasterbot"
        When I run "osrm-extract {osm_file} -p {profile_file}"
        Then stdout should contain "evaluating segment"
        And I run "osrm-contract {processed_file}"
        And I route I should get
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

    Scenario: Weighting based on raster sources
        Given the profile "rasterbotinterp"
        When I run "osrm-extract {osm_file} -p {profile_file}"
        Then stdout should contain "evaluating segment"
        And I run "osrm-contract {processed_file}"
        And I route I should get
            | from | to | route    | speed   |
            | a    | b  | ab,ab    | 8 km/h  |
            | a    | c  | ad,dc,dc | 15 km/h |
            | b    | c  | bc,bc    | 8 km/h  |
            | a    | d  | ad,ad    | 15 km/h |
            | d    | c  | dc,dc    | 15 km/h |
            | d    | e  | de,de    | 10 km/h |
            | e    | b  | eb,eb    | 10 km/h |
            | d    | f  | df,df    | 15 km/h |
            | f    | b  | fb,fb    | 7 km/h  |
            | d    | b  | de,eb,eb | 10 km/h |
