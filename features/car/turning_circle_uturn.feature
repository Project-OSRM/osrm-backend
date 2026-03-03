@routing @car @turning_circle @uturn
Feature: Car - Use turning circles for u-turns

    Background:
        Given the profile "car"

    Scenario: Car - Should use turning_circle for u-turn when direct u-turn is restricted
        Given the node map
            """
            a---b---c
                |
                d
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |

        And the nodes
            | node | highway        |
            | d    | turning_circle |

        And the relations
            | type        | way:from | node:via | way:to | restriction |
            | restriction | abc      | b        | abc    | no_u_turn   |

        When I route I should get
            | waypoints | bearings     | route                 | turns                                             |
            | a,a       | 90,10 270,10 | abc,bd,bd,abc,abc     | depart,turn right,continue uturn,turn left,arrive |

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
