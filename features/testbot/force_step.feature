@routing @testbot @via
Feature: Force routing steps
    Background:
        Given the profile "testbot"

    Scenario: Direct routes with waypoints on same edge
        Given the node map
            """
              1   2
            a-------b
            |       |
            d-------c
            |       |
            e-------f
              3   4
            """

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | ad    | no     |
            | bc    | no     |
            | cf    | no     |
            | dc    | no     |
            | de    | no     |
            | ef    | yes    |

        When I route I should get
            | waypoints | approaches        | weight | route          |
            | 1,2       |                   | 20     | ab,ab          |
            | 1,2       | curb curb         | 100    | ab,ad,dc,bc,ab |
            | 2,1       |                   | 20     | ab,ab          |
            | 2,1       | opposite opposite | 100    | ab,bc,dc,ad,ab |
            | 3,4       |                   | 20     | ef,ef          |
            | 4,3       |                   | 100    | ef,cf,dc,de,ef |

    Scenario: Via routes with waypoints on same edge
        Given the node map
            """
              1   2
            a-------b
            |       |
            d-5-----c
            |       |
            e-------f
              3   4
            """

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | ad    | no     |
            | bc    | no     |
            | cf    | no     |
            | dc    | no     |
            | de    | no     |
            | ef    | yes    |

        When I route I should get
            | waypoints | approaches                     | weight | route                      |
            | 5,1,2     |                                | 59.8   | dc,ad,ab,ab,ab             |
            | 5,1,2     | unrestricted curb curb         | 180.2  | dc,bc,ab,ab,ab,ad,dc,bc,ab |
            | 5,2,1     |                                | 80.2   | dc,bc,ab,ab,ab             |
            | 5,2,1     | unrestricted opposite opposite | 159.8  | dc,ad,ab,ab,ab,bc,dc,ad,ab |
            | 5,3,4     |                                | 59.8   | dc,de,ef,ef,ef             |
            | 5,4,3     |                                | 159.8  | dc,de,ef,ef,ef,cf,dc,de,ef |


    Scenario: [U-turns allowed] Via routes with waypoints on same edge
        Given the node map
            """
              1   2
            a-------b
            |       |
            d-5-----c
            |       |
            e-------f
              3   4
            """

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | ad    | no     |
            | bc    | no     |
            | cf    | no     |
            | dc    | no     |
            | de    | no     |
            | ef    | yes    |

        And the query options
            | continue_straight | false |

        When I route I should get
            | waypoints | approaches                     | weight | route                      |
            | 5,1,2     |                                | 59.8   | dc,ad,ab,ab,ab             |
            | 5,1,2     | unrestricted curb curb         | 180.2  | dc,bc,ab,ab,ab,ad,dc,bc,ab |
            | 5,2,1     |                                | 79.8   | dc,ad,ab,ab,ab,ab          |
            | 5,2,1     | unrestricted opposite opposite | 159.8  | dc,ad,ab,ab,ab,bc,dc,ad,ab |
            | 5,3,4     |                                | 59.8   | dc,de,ef,ef,ef             |
            | 5,4,3     |                                | 159.8  | dc,de,ef,ef,ef,cf,dc,de,ef |
