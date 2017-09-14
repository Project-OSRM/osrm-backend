@routing @car @relations
Feature: Car - route relations
    Background:
        Given the profile "car"

    @sliproads
    Scenario: Cardinal direction assignment to refs
        Given the node map
            """
                     a  b
                     |  |
              c------+--+------d
              e------+--+------f
                     |  |
                     g  h

              i----------------j
              k----------------l

              x----------------y
              z----------------w
            """

        And the ways
            | nodes | name        | highway  | ref         |
            | ag    | southbound  | motorway | I 80        |
            | hb    | northbound  | motorway | I 80        |
            | dc    | westbound   | motorway | I 85;CO 93  |
            | ef    | eastbound   | motorway | I 85;US 12  |
            | ij    | westbound-2 | motorway | I 99        |
            | ji    | eastbound-2 | motorway | I 99        |
            | kl    | eastbound-2 | motorway | I 99        |
            | lk    | eastbound-2 | motorway | I 99        |
            | xy    | watermill   | motorway | I 45M; US 3 |

        And the relations
            | type        | way:south | route | ref |
            | route       | ag        | road  | 80  |
            | route       | ef        | road  | 12  |

        And the relations
            | type        | way:north | route | ref |
            | route       | hb        | road  | 80  |
            | route       | cd        | road  | 93  |

        And the relations
            | type        | way:west | route | ref  |
            | route       | dc       | road  | 85   |
            | route       | ij       | road  | 99   |
            | route       | xy       | road  | I 45 |

        And the relations
            | type        | way:east | route | ref   |
            | route       | lk       | road  | I 99  |

        And the relations
            | type        | way:east | route | ref   |
            | route       | xy       | road  | US 3  |

        When I route I should get
            | waypoints | route                   | ref                                               |
            | a,g       | southbound,southbound   | I 80 $south,I 80 $south                           |
            | h,b       | northbound,northbound   | I 80 $north,I 80 $north                           |
            | d,c       | westbound,westbound     | I 85 $west; CO 93 $north,I 85 $west; CO 93 $north |
            | e,f       | eastbound,eastbound     | I 85; US 12 $south,I 85; US 12 $south             |
            | i,j       | westbound-2,westbound-2 | I 99 $west,I 99 $west                             |
            | l,k       | eastbound-2,eastbound-2 | I 99 $east,I 99 $east                             |
            | x,y       | watermill,watermill     | I 45M $west; US 3 $east,I 45M $west; US 3 $east   |