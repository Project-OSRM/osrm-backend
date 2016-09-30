@routing @continue_straight @via @testbot
Feature: U-turns at via points

    Background:
        Given the profile "testbot"
        Given a grid size of 250 meters

    Scenario: Continue straight at waypoints enabled by default
        Given the node map
            """
            a b c d
              e f g
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | be    |
            | dg    |
            | ef    |
            | fg    |

        When I route I should get
            | waypoints | route                   |
            | a,e,c     | ab,be,be,ef,fg,dg,cd,cd |

    Scenario: Query parameter to disallow changing direction at all waypoints
        Given the node map
            """
            a b c d
              e f g
            """

        And the query options
            | continue_straight | false |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | be    |
            | dg    |
            | ef    |
            | fg    |

        When I route I should get
            | waypoints | route             |
            | a,e,c     | ab,be,be,be,bc,bc |

    Scenario: Instructions at waypoints at u-turns
        Given the node map
            """
            a b c d
              e f g
            """

        And the query options
            | continue_straight | false |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | be    |
            | dg    |
            | ef    |
            | fg    |

        When I route I should get
            | waypoints | route             |
            | a,e,c     | ab,be,be,be,bc,bc |

    Scenario: u-turn mixed with non-uturn vias
        Given the node map
            """
            a 1 b 3 c 5 d
                2       4
                e   f   g
            """

        And the query options
            | continue_straight | false |

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | bc    | no     |
            | cd    | no     |
            | be    | yes    |
            | dg    | no     |
            | ef    | no     |
            | fg    | no     |

        When I route I should get
            | waypoints | route                                           |
            | 1,2,3,4,5 | ab,be,be,be,ef,fg,dg,cd,bc,bc,bc,cd,dg,dg,dg,cd,cd |

