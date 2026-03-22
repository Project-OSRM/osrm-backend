@routing @foot @countryspeeds
Feature: Foot - Country-specific highway access

  Background:
    Given the extract extra arguments "--location-dependent-data test/data/countrytest.geojson"
    And the partition extra arguments "--threads 1"
    And the customize extra arguments "--threads 1"
    And the profile file "foot" initialized with
    """
    profile.uselocationtags.countryspeeds = true
    """

  Scenario: Foot - Trunk blocked where prohibited by country rules

    And the node locations
      | node | lat | lon  |
      | a    | 8.5 | 5.0  |
      | b    | 8.5 | 10.0 |
      | c    | 8.5 | 15.0 |
      | d    | 7.5 | 5.0  |
      | e    | 7.5 | 10.0 |
      | f    | 7.5 | 15.0 |

    And the ways
      | nodes | highway |
      | ab    | trunk   |
      | bc    | trunk   |
      | de    | trunk   |
      | ef    | trunk   |

    When I route I should get
      | waypoints | route | status | message                         |
      | a,b       |       | 400    | Impossible route between points |
      | b,c       |       | 400    | Impossible route between points |
      | d,e       | de,de | 200    |                                 |
      | e,f       | ef,ef | 200    |                                 |
