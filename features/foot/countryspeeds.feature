@routing @foot @countryspeeds
Feature: Foot - Country-specific highway access

  Background:
    Given the extract extra arguments "--location-dependent-data test/data/country"
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

  Scenario: Foot - Trunk link also blocked where trunk is prohibited

    And the node locations
      | node | lat | lon  |
      | a    | 8.5 | 5.0  |
      | b    | 8.5 | 6.0  |
      | c    | 7.5 | 5.0  |
      | d    | 7.5 | 6.0  |

    And the ways
      | nodes | highway    |
      | ab    | trunk_link |
      | cd    | trunk_link |

    When I route I should get
      | waypoints | route | status | message                         |
      | a,b       |       | 400    | Impossible route between points |
      | c,d       | cd,cd | 200    |                                 |

  Scenario: Foot - Cycleway accessible in Switzerland and Finland

    # CHE (lat 9-10) and FIN (lat 9-10, lon 10-20) allow foot on cycleways.
    # GRC (lat 7-8) follows Worldwide defaults → cycleway not accessible.
    And the node locations
      | node | lat | lon  |
      | a    | 9.5 | 5.0  |
      | b    | 9.5 | 6.0  |
      | c    | 9.5 | 15.0 |
      | d    | 9.5 | 16.0 |
      | e    | 7.5 | 5.0  |
      | f    | 7.5 | 6.0  |

    And the ways
      | nodes | highway  |
      | ab    | cycleway |
      | cd    | cycleway |
      | ef    | cycleway |

    When I route I should get
      | waypoints | route | status | message                         |
      | a,b       | ab,ab | 200    |                                 |
      | c,d       | cd,cd | 200    |                                 |
      | e,f       |       | 400    | Impossible route between points |

  Scenario: Foot - Bridleway accessible in Belgium and Ireland

    # BEL (lat 8-9, lon 10-20): bridleway allowed.
    # IRL (lat 7-8, lon 10-20): bridleway allowed.
    # FRA (lat 8-9, lon 0-10): trunk blocked but bridleway still blocked.
    And the node locations
      | node | lat | lon  |
      | a    | 8.5 | 15.0 |
      | b    | 8.5 | 16.0 |
      | c    | 7.5 | 15.0 |
      | d    | 7.5 | 16.0 |
      | e    | 8.5 | 5.0  |
      | f    | 8.5 | 6.0  |

    And the ways
      | nodes | highway   |
      | ab    | bridleway |
      | cd    | bridleway |
      | ef    | bridleway |

    When I route I should get
      | waypoints | route | status | message                         |
      | a,b       | ab,ab | 200    |                                 |
      | c,d       | cd,cd | 200    |                                 |
      | e,f       |       | 400    | Impossible route between points |

  Scenario: Foot - Unknown country code falls back to Worldwide defaults

    # LTU polygon is loaded but LTU is not in the known-country set;
    # falls back to Worldwide → trunk accessible, cycleway not accessible.
    And the node locations
      | node | lat | lon |
      | a    | 6.5 | 2.0 |
      | b    | 6.5 | 4.0 |
      | c    | 6.5 | 6.0 |
      | d    | 6.5 | 8.0 |

    And the ways
      | nodes | highway  |
      | ab    | trunk    |
      | cd    | cycleway |

    When I route I should get
      | waypoints | route | status | message                         |
      | a,b       | ab,ab | 200    |                                 |
      | c,d       |       | 400    | Impossible route between points |

  Scenario: Foot - Trunk accessible worldwide when outside all country polygons

    And the node locations
      | node | lat  | lon  |
      | a    | 11.0 | 5.0  |
      | b    | 11.0 | 15.0 |

    And the ways
      | nodes | highway |
      | ab    | trunk   |

    When I route I should get
      | waypoints | route | status |
      | a,b       | ab,ab | 200    |

  Scenario: Foot - Overlapping polygons both block trunk — routing is consistent

    # Two polygons overlap (CHE lat 5-10, FRA lat 0-8, overlap lat 5-8).
    # Both countries block trunk for pedestrians, so whichever polygon wins
    # the PIP test trunk remains inaccessible.
    Given the extract extra arguments "--location-dependent-data test/data/country-overlap.geojson"

    And the node locations
      | node | lat | lon |
      | a    | 6.5 | 2.0 |
      | b    | 6.5 | 8.0 |

    And the ways
      | nodes | highway |
      | ab    | trunk   |

    When I route I should get
      | waypoints | route | status | message                         |
      | a,b       |       | 400    | Impossible route between points |


