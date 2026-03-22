@routing @bicycle @countryspeeds
Feature: Bicycle - Country-specific highway access

  Background:
    Given the extract extra arguments "--location-dependent-data test/data/country"
    And the partition extra arguments "--threads 1"
    And the customize extra arguments "--threads 1"
    And the profile file "bicycle" initialized with
    """
    profile.uselocationtags.countryspeeds = true
    """

  Scenario: Bicycle - Trunk blocked where prohibited by country rules

    And the node locations
      | node | lat | lon  |
      | a    | 9.5 | 5.0  |
      | b    | 9.5 | 10.0 |
      | c    | 9.5 | 15.0 |
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

  Scenario: Bicycle - Bridleway accessible where permitted by country rules

    And the node locations
      | node | lat | lon  |
      | a    | 7.5 | 5.0  |
      | b    | 7.5 | 10.0 |
      | c    | 7.5 | 15.0 |

    And the ways
      | nodes | highway   |
      | ab    | bridleway |
      | bc    | bridleway |

    When I route I should get
      | waypoints | route | status |
      | a,b       | ab,ab | 200    |
      | b,c       | bc,bc | 200    |

  Scenario: Bicycle - Trunk accessible worldwide when outside all country polygons

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

