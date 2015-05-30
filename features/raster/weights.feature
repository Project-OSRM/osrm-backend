@routing @speed @raster
Feature: Raster - weights

    Background: Use specific speeds
        Given the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | .05        | 0.1      |
            | c    | 0.0        | 0.1      |
            | d    | .05        | .03      |
            | e    | .05        | .066     |
            | f    | .075       | .066     |
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

    Scenario: Weighting not based on raster sources
        Given the profile "testbot"
        When I run "osrm-extract {osm_base}.osm -p {profile}"
        And I run "osrm-prepare {osm_base}.osm"
        And I route I should get
            | from | to | route | speed   |
            | a    | b  | ab    | 36 km/h |
            | a    | c  | ab,bc | 36 km/h |
            | b    | c  | bc    | 36 km/h |
            | a    | d  | ad    | 36 km/h |
            | d    | c  | dc    | 36 km/h |

    Scenario: Weighting based on raster sources
        Given the profile "rasterbot"
        When I run "osrm-extract {osm_base}.osm -p {profile}"
        Then stdout should contain "evaluating segment"
        And I run "osrm-prepare {osm_base}.osm"
        And I route I should get
            | from | to | route | speed   |
            | a    | b  | ab    | 8 km/h  |
            | a    | c  | ad,dc | 15 km/h |
            | b    | c  | bc    | 8 km/h  |
            | a    | d  | ad    | 15 km/h |
            | d    | c  | dc    | 15 km/h |
            | d    | e  | de    | 10 km/h |
            | e    | b  | eb    | 10 km/h |
            | d    | f  | df    | 15 km/h |
            | f    | b  | fb    | 7 km/h  |
            | d    | b  | de,eb | 10 km/h |

    Scenario: Weighting based on raster sources
        Given the profile "rasterbot-interp"
        When I run "osrm-extract {osm_base}.osm -p {profile}"
        Then stdout should contain "evaluating segment"
        And I run "osrm-prepare {osm_base}.osm"
        And I route I should get
            | from | to | route | speed   |
            | a    | b  | ab    | 8 km/h  |
            | a    | c  | ad,dc | 15 km/h |
            | b    | c  | bc    | 8 km/h  |
            | a    | d  | ad    | 15 km/h |
            | d    | c  | dc    | 15 km/h |
            | d    | e  | de    | 10 km/h |
            | e    | b  | eb    | 10 km/h |
            | d    | f  | df    | 15 km/h |
            | f    | b  | fb    | 7 km/h  |
            | d    | b  | de,eb | 10 km/h |
