Feature: Invalid profile API versions

    Background:
        Given a grid size of 100 meters

    Scenario: Profile API version too low
        Given the profile file
          """
          api_version = -1
          """
        And the node map
          """
            ab
          """
        And the ways
            | nodes  |
            | ab     |
        And the data has been saved to disk

        When I try to run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit with an error
        And stderr should contain "Invalid profile API version"

    Scenario: Profile API version too high
        Given the profile file
          """
          api_version = 5
          """
        And the node map
          """
            ab
          """
        And the ways
            | nodes  |
            | ab     |
        And the data has been saved to disk

        When I try to run "osrm-extract --profile {profile_file} {osm_file}"
        Then it should exit with an error
        And stderr should contain "Invalid profile API version"
