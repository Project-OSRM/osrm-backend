@routing @testbot @oneway
Feature: Handle multiple phantom nodes in one-way segment

# Check we handle routes where source and destination are
# phantom nodes on the same one-way segment.
# See: https://github.com/Project-OSRM/osrm-backend/issues/5788

    Background:
    Given the profile "testbot"

    Scenario: One-way segment with adjacent phantom nodes
    Given the node map
        """
        d  c

        a12b
        """

    And the ways
       | nodes | oneway |
       | ab    | yes    |
       | bc    | no     |
       | cd    | no     |
       | da    | no     |

    When I route I should get
       | from | to | route          | time      | distance |
       | 1    | 2  | ab,ab          | 5s  +-0.1 | 50m  ~1% |
       | 1    | c  | ab,bc,bc       | 30s +-0.1 | 300m ~1% |
       | 2    | 1  | ab,bc,cd,da,ab | 65s +-0.1 | 650m ~1% |
       | 2    | c  | ab,bc,bc       | 25s +-0.1 | 250m ~1% |
       | c    | 1  | cd,da,ab       | 40s +-0.1 | 400m ~1% |
       | c    | 2  | cd,da,ab       | 45s +-0.1 | 450m ~1% |

    When I request a travel time matrix I should get
       |   | 1        | 2        | c         |
       | 1 | 0        | 5  +-0.1 | 30 +-0.1  |
       | 2 | 65 +-0.1 | 0        | 25 +-0.1  |
       | c | 40 +-0.1 | 45 +-0.1 | 0         |

    When I request a travel time matrix I should get
       |   | 1  | 2       | c        |
       | 1 | 0  | 5 +-0.1 | 30 +-0.1 |

    When I request a travel time matrix I should get
       |   | 1        | 2  | c        |
       | 2 | 65 +-0.1 | 0  | 25 +-0.1 |

    When I request a travel time matrix I should get
       |   | 1        |
       | 1 | 0        |
       | 2 | 65 +-0.1 |
       | c | 40 +-0.1 |

    When I request a travel time matrix I should get
       |   | 2        |
       | 1 | 5  +-0.1 |
       | 2 | 0        |
       | c | 45 +-0.1 |

    When I request a travel distance matrix I should get
       |   | 1       | 2       | c       |
       | 1 | 0       | 50  ~1% | 300 ~1% |
       | 2 | 650 ~1% | 0       | 250 ~1% |
       | c | 400 ~1% | 450 ~1% | 0       |

    When I request a travel distance matrix I should get
       |   | 1   | 2      | c       |
       | 1 | 0   | 50 ~1% | 300 ~1% |

    When I request a travel distance matrix I should get
       |   | 1       | 2   | c       |
       | 2 | 650 ~1% | 0   | 250 ~1% |

    When I request a travel distance matrix I should get
       |   | 1       |
       | 1 | 0       |
       | 2 | 650 ~1% |
       | c | 400 ~1% |

    When I request a travel distance matrix I should get
       |   | 2       |
       | 1 | 50  ~1% |
       | 2 | 0       |
       | c | 450 ~1% |
