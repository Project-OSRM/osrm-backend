@routing @foot @countryway
Feature: Foot - Graceful fallback when no country data provided

  Background:
    Given the profile file "foot" initialized with
    """
    profile.uselocationtags.countryspeeds = true
    """

  Scenario: Foot - Profile defaults apply when no location data files are loaded
    Then routability should be
      | highway       | forw |
      | motorway      |      |
      | motorway_link |      |
      | trunk         |      |
      | trunk_link    |      |
      | primary       | x    |
      | secondary     | x    |
      | tertiary      | x    |
      | residential   | x    |
      | footway       | x    |
      | pedestrian    | x    |
      | steps         | x    |
      | path          | x    |
      | cycleway      |      |
      | bridleway     |      |

