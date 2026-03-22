@routing @bicycle @countryway
Feature: Bicycle - Graceful fallback when no country data provided

  Background:
    Given the profile file "bicycle" initialized with
    """
    profile.uselocationtags.countryspeeds = true
    """

  Scenario: Bicycle - Profile defaults apply when no location data files are loaded
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
      | cycleway      | x    |
      | footway       | x    |
      | bridleway     |      |
      | path          | x    |

  Scenario: Bicycle - Bridleway not accessible without country data
    Then routability should be
      | highway   | forw |
      | bridleway |      |

