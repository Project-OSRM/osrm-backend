@raster @extract
Feature: osrm-extract with a profile containing raster source
    Scenario: osrm-extract on a valid profile
        Given the profile "rasterbot"
        And the node locations
            | node | lat        | lon      |
            | a    | 0.1        | 0.1      |
            | b    | 0.05       | 0.1      |
        And the ways
            | nodes |
            | ab    |
        And the raster source
            """
            0  0  0   0
            0  0  0   250
            0  0  250 500
            0  0  0   250
            0  0  0   0
            """
        And the data has been saved to disk
        When I run "osrm-extract {osm_file} -p {profile_file}"
        Then stdout should contain "source loader"
        Then stdout should contain "slope: 0.0904"
        Then stdout should contain "slope: -0.0904"
        And it should exit successfully
