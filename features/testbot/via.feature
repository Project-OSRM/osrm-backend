@routing @testbot @via
Feature: Via points

    Background:
        Given the profile "testbot"

    Scenario: Simple via point
        Given the node map
            | a | b | c |

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | waypoints | route |
            | a,b,c     | abc   |
            | c,b,a     | abc   |

    Scenario: Via point at a dead end
        Given the node map
            | a | b | c |
            |   | d |   |

        And the ways
            | nodes |
            | abc   |
            | bd    |

        When I route I should get
            | waypoints | route         |
            | a,d,c     | abc,bd,bd,abc |
            | c,d,a     | abc,bd,bd,abc |

    Scenario: Multiple via points
        Given the node map
            | a |   |   |   | e | f | g |   |
            |   | b | c | d |   |   |   | h |

        And the ways
            | nodes |
            | ae    |
            | ab    |
            | bcd   |
            | de    |
            | efg   |
            | gh    |
            | dh    |

        When I route I should get
            | waypoints | route            |
            | a,c,f     | ab,bcd,de,efg    |
            | a,c,f,h   | ab,bcd,de,efg,gh |
    
    @bug
    Scenario: Via points on ring of oneways
        Given the node map
            | a | 1 | 2 | 3 | b |
            | d |   |   |   | c |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                      | distance | turns                                                      |
            | 1,3       | ab                         | 200m +-1 | head,destination                                           |
            | 3,1       | ab,bc,cd,da,ab             | 800m +-1 | head,right,right,right,right,destination                   |
            | 1,2,3     | ab                         | 200m +-1 | head,destination                                           |
            | 1,3,2     | ab,bc,cd,da,ab             | 1100m +- | head,right,right,right,right,destination                   |
            | 3,2,1     | ab,bc,cd,da,ab,bc,cd,da,ab | 1900m +- | head,right,right,right,right,right,right,right,destination |
