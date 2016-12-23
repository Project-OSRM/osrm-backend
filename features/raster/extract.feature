@raster @extract
Feature: osrm-extract with a profile containing raster source
    Scenario: osrm-extract on a valid profile
        Given the profile "rasterbot.lua"
        And the node map
            """
            a b
            """
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
        And it should exit successfully
