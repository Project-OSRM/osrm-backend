@raster @extract
Feature: osrm-extract with a profile containing raster source
# expansions:
# {osm_base} => path to current input file
# {profile} => path to current profile script

    Scenario: osrm-extract on a valid profile
        Given the profile "rasterbot"
        And the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk
        When I run "osrm-extract {osm_base}.osm -p {profile}"
        Then stderr should be empty
        And stdout should contain "source loader"
        And it should exit successfully
