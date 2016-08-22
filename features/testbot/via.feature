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
            | waypoints | route           |
            | a,b,c     | abc,abc,abc,abc |

    Scenario: Simple via point with core factor
        Given the contract extra arguments "--core 0.8"
        Given the node map
            | a | b | c |

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | waypoints | route           |
            | a,b,c     | abc,abc,abc,abc |
            | c,b,a     | abc,abc,abc,abc |
            | c,b,a     | abc,abc,abc,abc |

    Scenario: Via point at a dead end
        Given the node map
            | a | b | c |
            |   | d |   |

        And the ways
            | nodes |
            | abc   |
            | bd    |

        When I route I should get
            | waypoints | route                |
            | a,d,c     | abc,bd,bd,bd,abc,abc |
            | c,d,a     | abc,bd,bd,bd,abc,abc |

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
            | waypoints | route                               |
            | a,c,f     | ab,bcd,bcd,bcd,de,efg,efg           |
            | a,c,f,h   | ab,bcd,bcd,bcd,de,efg,efg,efg,gh,gh |


    Scenario: Duplicate via point
        Given the node map
            | x |   |   |   |   |   |
            | a | 1 | 2 | 3 | 4 | b |
            |   |   |   |   |   |   |

        And the ways
            | nodes |
            | xa    |
            | ab    |

        When I route I should get
            | waypoints | route       |
            | 1,1,4     | ab,ab,ab,ab |

    Scenario: Via points on ring of oneways
    # xa it to avoid only having a single ring, which cna trigger edge cases
        Given the node map
            |   | x |   |   |   |   |   | g |   |
            |   | a | 1 | b | 2 | c | 3 | d |   |
            | i | f |   |   |   |   |   | e | h |

        And the ways
            | nodes | oneway |
            | xa    |        |
            | if    |        |
            | gd    |        |
            | eh    |        |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fa    | yes    |

        When I route I should get
            | waypoints | route                                     | distance  |
            | 1,3       | ab,bc,cd,cd                               |  400m +-1 |
            | 3,1       | cd,de,ef,fa,ab,ab                         | 1000m +-1 |
            | 1,2,3     | ab,bc,bc,bc,cd,cd                         |  400m +-1 |
            | 1,3,2     | ab,bc,cd,cd,cd,de,ef,fa,ab,bc,bc          | 1600m +-1 |
            | 3,2,1     | cd,de,ef,fa,ab,bc,bc,bc,cd,de,ef,fa,ab,ab | 2400m +-1 |

    Scenario: Via points on ring on the same oneway
    # xa it to avoid only having a single ring, which cna trigger edge cases
        Given the node map
            |   | x |   |   |   | e |   |
            |   | a | 1 | 2 | 3 | b |   |
            | g | d |   |   |   | c | f |

        And the ways
            | nodes | oneway |
            | xa    |        |
            | eb    |        |
            | cf    |        |
            | dg    |        |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                               | distance  |
            | 1,3       | ab,ab                               | 200m +-1  |
            | 3,1       | ab,bc,cd,da,ab,ab                   | 800m +-1  |
            | 1,2,3     | ab,ab,ab,ab                         | 200m +-1  |
            | 1,3,2     | ab,ab,ab,bc,cd,da,ab,ab             | 1100m +-1 |
            | 3,2,1     | ab,bc,cd,da,ab,ab,ab,bc,cd,da,ab,ab | 1800m +-1 |

    # See issue #1896
    Scenario: Via point at a dead end with oneway
        Given the node map
            | a | b | c |
            |   | d |   |
            |   | e |   |

        And the ways
            | nodes | oneway |
            | abc   |  no    |
            | bd    |  no    |
            | de    |  yes   |

        When I route I should get
            | waypoints | route                |
            | a,d,c     | abc,bd,bd,bd,abc,abc |
            | c,d,a     | abc,bd,bd,bd,abc,abc |

    # See issue #2349
    Scenario: Via point at a dead end with oneway
        Given the node map
            | a | b | c |
            |   | d |   |
            |   | e |   |

        And the ways
            | nodes | oneway |
            | abc   |  no    |
            | bd    |  no    |
            | ed    |  yes   |

        When I route I should get
            | waypoints | route                |
            | a,d,c     | abc,bd,bd,bd,abc,abc |
            | c,d,a     | abc,bd,bd,bd,abc,abc |

    # See issue #2349
    @bug @todo
    Scenario: Via point at a dead end with oneway
        Given the node map
            | a | b | c |
            |   | d |   |
            |   | e | g |
            |   | f |   |

        And the ways
            | nodes | oneway |
            | abc   |  no    |
            | bd    |  no    |
            | ed    |  yes   |
            | dg    |  yes   |
            | ef    |  no    |
            | fg    |  yes   |

        When I route I should get
            | waypoints | route                |
            | a,d,c     | abc,bd,bd,bd,abc,abc |
            | c,d,a     | abc,bd,bd,bd,abc,abc |

    # See issue #1896
    Scenario: Via point at a dead end with barrier
        Given the profile "car"
        Given the node map
            | a | b | c |
            |   | 1 |   |
            |   | d |   |
            |   |   |   |
            |   |   |   |
            | f | e |   |

        And the nodes
            | node | barrier |
            | d    | bollard |

        And the ways
            | nodes |
            | abc   |
            | bd    |
            | afed  |

        When I route I should get
            | waypoints | route                   |
            | a,1,c     | abc,bd,bd,bd,bd,abc,abc |
            | c,1,a     | abc,bd,bd,bd,bd,abc,abc |

    Scenario: Via points on ring on the same oneway, forces one of the vertices to be top node
        Given the node map
            | a | 1 | 2 | b |
            | 8 |   |   | 3 |
            | 7 |   |   | 4 |
            | d | 6 | 5 | c |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                      | distance   |
            | 2,1       | ab,bc,cd,da,ab,ab          | 1100m +-1  |
            | 4,3       | bc,cd,da,ab,bc,bc          | 1100m +-1  |
            | 6,5       | cd,da,ab,bc,cd,cd          | 1100m +-1  |
            | 8,7       | da,ab,bc,cd,da,da          | 1100m +-1  |

    Scenario: Multiple Via points on ring on the same oneway, forces one of the vertices to be top node
        Given the node map
            | a | 1 | 2 | 3 | b |
            |   |   |   |   | 4 |
            |   |   |   |   | 5 |
            |   |   |   |   | 6 |
            | d | 9 | 8 | 7 | c |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                               | distance     |
            | 3,2,1     | ab,bc,cd,da,ab,ab,ab,bc,cd,da,ab,ab | 3000m +-1    |
            | 6,5,4     | bc,cd,da,ab,bc,bc,bc,cd,da,ab,bc,bc | 3000m +-1    |
            | 9,8,7     | cd,da,ab,bc,cd,cd,cd,da,ab,bc,cd,cd | 3000m +-1    |

    # See issue #2706
    Scenario: Incorrect ordering of nodes can produce multiple U-turns
        Given the node map
            |   | a |   |   |   |
            | e | b | c | d | f |

        And the ways
            | nodes  | oneway |
            | abcd   | no     |
            | ebbdcf | yes    |

        When I route I should get
            | from | to | route         |
            | e    | f  | ebbdcf,ebbdcf |

    @2798
    Scenario: UTurns Enabled
        Given the node map
            | a | b | c | d | e |

        And the query options
            | continue_straight | false |

        And the ways
            | nodes | oneway |
            | abc   | yes    |
            | edc   | yes    |

        When I route I should get
            | waypoints | route |
            | a,b,e     |       |
