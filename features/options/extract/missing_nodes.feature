@extract @options @missing_nodes
Feature: osrm-extract - Missing node references

    Background:
        Given the profile "testbot"

    Scenario: osrm-extract - Warn about missing node references
        Given the node map
            """
            a b c
            """
        And the ways
            | nodes |
            | abc   |
        And node "b" is removed from OSM data
        And the data has been saved to disk
        When I try to run "osrm-extract {osm_file} --profile {profile_file}"
        Then it should exit with an error
        And stderr should contain "referenced by ways were not found"
