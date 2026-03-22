@routing @foot @countryway
Feature: Foot - Worldwide access with countryspeeds enabled

  Background:
    Given the profile file "foot" initialized with
    """
    profile.uselocationtags.countryspeeds = true
    """

  Scenario: Foot - Trunk accessible worldwide when countryspeeds enabled
    Then routability should be
      | highway       | forw |
      | motorway      |      |
      | motorway_link |      |
      | trunk         | x    |
      | trunk_link    | x    |
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
