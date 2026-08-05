@routing @car @turning_circle @uturn
Feature: Car - Use turning circles for u-turns

    Background:
        Given the profile "car"

    Scenario: Car - Should use turning_circle for u-turn when direct u-turn is restricted
        Given the node map
            """
            a---b---c---e
                |
                d
            """

        And the ways
            | nodes | highway |
            | abce  | primary |
            | bd    | primary |

        And the nodes
            | node | highway        |
            | d    | turning_circle |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | abce     | b        | abce   | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                   | turns                                             |
            | a,a       | 90,10 270,10 | abce,bd,bd,abce,abce    | depart,turn right,continue uturn,turn left,arrive |

    Scenario: Car - Should use turning_loop for u-turn when direct u-turn is restricted
        Given the node map
            """
            a---b---c---d
                |
                e
            """

        And the ways
            | nodes | highway |
            | abcd  | primary |
            | be    | primary |

        And the nodes
            | node | highway      |
            | e    | turning_loop |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | abcd     | b        | abcd   | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                   | turns                                             |
            | a,a       | 90,10 270,10 | abcd,be,be,abcd,abcd    | depart,turn right,continue uturn,turn left,arrive |

    Scenario: Car - Should use mini_roundabout for u-turn when direct u-turn is restricted
        Given the node map
            """
            a---b---c---d
                |
                e
            """

        And the ways
            | nodes | highway |
            | abcd  | primary |
            | be    | primary |

        And the nodes
            | node | highway         |
            | e    | mini_roundabout |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | abcd     | b        | abcd   | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                   | turns                                             |
            | a,a       | 90,10 270,10 | abcd,be,be,abcd,abcd    | depart,turn right,continue uturn,turn left,arrive |

    Scenario: Car - Prefer turning_circle over plain dead end for u-turn
        Given the node map
            """
                c
                |
            a---b
                |
                d
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | bd    | primary |

        And the nodes
            | node | highway        |
            | d    | turning_circle |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | ab       | b        | ab     | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route             | turns                                             |
            | a,a       | 90,10 270,10 | ab,bd,bd,ab,ab    | depart,turn right,continue uturn,turn left,arrive |

    Scenario: Car - Multiple turning facilities, use closest
        Given the node map
            """
            a---b---c---d---e
                |       |
                f       g
            """

        And the ways
            | nodes | highway |
            | abcde | primary |
            | bf    | primary |
            | dg    | primary |

        And the nodes
            | node | highway        |
            | f    | turning_circle |
            | g    | turning_loop   |

        When I route I should get
            | waypoints | bearings     | route                       | turns                                             |
            | a,a       | 90,10 270,10 | abcde,bf,bf,abcde,abcde     | depart,turn right,continue uturn,turn left,arrive |

    Scenario: Car - Dead end with turning_circle
        Given the node map
            """
            a---b---c
                    |
                    d
            """

        And the ways
            | nodes | highway   |
            | abc   | primary   |
            | cd    | primary   |

        And the nodes
            | node | highway        |
            | d    | turning_circle |

        When I route I should get
            | waypoints | route       | turns                            |
            | a,d       | abc,cd,cd   | depart,new name right,arrive     |
            | d,a       | cd,abc,abc  | depart,new name left,arrive      |

    Scenario: Car - Regular u-turn without turning facility still penalized
        Given the node map
            """
            a---b---c---d
            """

        And the ways
            | nodes | highway |
            | abcd  | primary |

        # Note: No turning facility nodes defined

        When I route I should get
            | waypoints | bearings     | route             | turns                        |
            | a,a       | 90,10 270,10 | abcd,abcd,abcd    | depart,continue uturn,arrive |

    Scenario: Car - Turning circle on one-way should respect direction
        Given the node map
            """
            a---b---c
                |
                d
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | no     |
            | bd    | primary | yes    |

        And the nodes
            | node | highway        |
            | d    | turning_circle |

        When I route I should get
            | waypoints | bearings     | route         | turns                        |
            | a,a       | 90,10 270,10 | abc,abc,abc   | depart,continue uturn,arrive |

    # Complex scenario: turning circle as an intermediate node on a through road,
    # not at a dead-end leaf. The turning circle sits between two road segments (c-f-g),
    # and vehicles can use it to turn around mid-journey.
    # no_u_turn at c prevents direct u-turn on Main St.
    # The turning circle at f provides a low-penalty u-turn point.
    Scenario: Car - Intermediate turning circle on through road used for u-turn
        Given the node map
            """
            a---b---c---d---e
                    |
                    f
                    |
                    g
            """

        And the ways
            | nodes | highway | name      |
            | ab    | primary | Main St   |
            | bc    | primary | Main St   |
            | cd    | primary | Main St   |
            | de    | primary | Main St   |
            | cf    | primary | Side Rd   |
            | fg    | primary | Side Rd   |

        And the nodes
            | node | highway        |
            | f    | turning_circle |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | bc       | c        | cd     | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                                    | turns                                             |
            | a,a       | 90,10 270,10 | Main St,Side Rd,Side Rd,Main St,Main St  | depart,turn right,continue uturn,turn left,arrive |

    # Complex scenario: one-way main road segment prevents returning the same way.
    # bc is one-way b→c, so once past b you cannot go back through c.
    # The turning circle at e (intermediate node between b and f) is the only practical
    # u-turn point, with its lower penalty making the detour worthwhile.
    Scenario: Car - One-way main road forces u-turn at intermediate turning circle
        Given the node map
            """
            a---b===c---d
                |
                e
                |
                f
            """

        And the ways
            | nodes | highway | oneway | name      |
            | ab    | primary |        | Main St   |
            | bc    | primary | yes    | Main St   |
            | cd    | primary |        | Main St   |
            | be    | primary |        | Side Rd   |
            | ef    | primary |        | Side Rd   |

        And the nodes
            | node | highway        |
            | e    | turning_circle |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | ab       | b        | ab     | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                                    | turns                                             |
            | a,a       | 90,10 270,10 | Main St,Side Rd,Side Rd,Main St,Main St  | depart,turn right,continue uturn,turn left,arrive |

    # Two competing intermediate turning facilities at different branches from the
    # same intersection. The router must choose the optimal one.
    # Bearings 90,10 270,10 force the east-west axis, ensuring consistent approach.
    Scenario: Car - Multiple intermediate turning facilities with bearings
        Given the node map
            """
            a---b---c---d---e
                |       |
                f       g
                |       |
                h       i
            """

        And the ways
            | nodes | highway | name      |
            | ab    | primary | Main St   |
            | bc    | primary | Main St   |
            | cd    | primary | Main St   |
            | de    | primary | Main St   |
            | bf    | primary | North Rd  |
            | fh    | primary | North Rd  |
            | dg    | primary | South Rd  |
            | gi    | primary | South Rd  |

        And the nodes
            | node | highway        |
            | f    | turning_circle |
            | g    | turning_loop   |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | ab       | b        | bc     | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                                                 | turns                                             |
            | a,a       | 90,10 270,10 | Main St,North Rd,North Rd,Main St,Main St             | depart,turn right,continue uturn,turn left,arrive |

    # Two competing u-turn paths from the same intersection.
    # Branch e-g has a turning_circle at intermediate node e (between b and g).
    # Branch f-h is a plain dead-end with no turning facility.
    # no_u_turn at b blocks the direct u-turn.
    # The router must choose: u-turn at turning circle e (5s), or continue to
    # dead-end g (20s), or go the long way via f-h (20s + further distance).
    # The turning circle e wins because its u-turn penalty is lowest (5s vs 20s).
    Scenario: Car - U-turn at intermediate turning circle beats competing dead-end paths
        Given the node map
            """
            a---b---c---d
                |       |
                e       f
                |       |
                g       h
            """

        And the ways
            | nodes | highway | name      |
            | ab    | primary | Main St   |
            | bc    | primary | Main St   |
            | cd    | primary | Main St   |
            | be    | primary | East Rd   |
            | eg    | primary | East Rd   |
            | df    | primary | West Rd   |
            | fh    | primary | West Rd   |

        And the nodes
            | node | highway        |
            | e    | turning_circle |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | ab       | b        | ab     | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                                    | turns                                             | time     | weight  |
            | a,a       | 90,10 270,10 | Main St,East Rd,East Rd,Main St,Main St  | depart,turn right,continue uturn,turn left,arrive | 59s +-10 | 59 +-10 |

    # Same topology as above but without the no_u_turn restriction.
    # U-turns at degree-3 intersections are already blocked by intersection_analysis,
    # so the restriction is not needed. The turning circle route is still chosen
    # because it's the only legal u-turn point (5s) vs continuing to dead-end g (20s)
    # or the longer detour via dead-end h (20s + extra distance).
    Scenario: Car - Turning circle u-turn still chosen without explicit restriction
        Given the node map
            """
            a---b---c---d
                |       |
                e       f
                |       |
                g       h
            """

        And the ways
            | nodes | highway | name      |
            | ab    | primary | Main St   |
            | bc    | primary | Main St   |
            | cd    | primary | Main St   |
            | be    | primary | East Rd   |
            | eg    | primary | East Rd   |
            | df    | primary | West Rd   |
            | fh    | primary | West Rd   |

        And the nodes
            | node | highway        |
            | e    | turning_circle |

        When I route I should get
            | waypoints | bearings     | route                                    | turns                                             | time     | weight  |
            | a,a       | 90,10 270,10 | Main St,East Rd,East Rd,Main St,Main St  | depart,turn right,continue uturn,turn left,arrive | 59s +-10 | 59 +-10 |
