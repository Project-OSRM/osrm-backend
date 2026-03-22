@routing @bicycle @countryway
Feature: Bicycle - Worldwide access with countryspeeds enabled

  Background:
    Given the profile file "bicycle" initialized with
    """
    profile.uselocationtags.countryspeeds = true
    """

  Scenario: Bicycle - Trunk accessible worldwide when countryspeeds enabled
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
      | cycleway      | x    |
      | footway       | x    |
      | bridleway     |      |
      | path          | x    |

  Scenario: Bicycle - Bridleway not accessible by default worldwide
    Then routability should be
      | highway   | forw |
      | bridleway |      |
