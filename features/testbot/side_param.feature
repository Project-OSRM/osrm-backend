@routing @side_param @testbot
Feature: Side parameter

    Background:
        Given the profile "car"
        And a grid size of 10 meters

    Scenario: Testbot - Start End same side, option both for Start and End
        Given the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b b   | ab,bc |

    Scenario: Testbot - Start End same side, option both for Start and Default for End
        Given the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | sides | route    |
            | s    | e  | b d   | ab,bc,bc |

    Scenario: Testbot - Start End same side, option both for Start and Opposite for End
        Given the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b o   | ab,bc |

    Scenario: Testbot - Start End opposite side, option both for Start and End
        Given the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b b   | ab,bc |

    Scenario: Testbot - Start End opposite side, option both for Start and Default for End
        Given the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b d   | ab,bc |

    Scenario: Testbot - Start End opposite side, option both for Start and Opposite for End
        Given the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | sides | route    |
            | s    | e  | b o   | ab,bc,bc |

    ###############
    # Oneway Test #
    ###############
    Scenario: Testbot - Test on oneway segment, Start End same side, option both for Start and End, 
        Given the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b b   | ab,bc |

    Scenario: Testbot - Test on oneway segment, Start End same side, option both for Start and Default for End
        Given the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b d   | ab,bc |

    Scenario: Testbot - Test on oneway segment, Start End same side, option both for Start and Opposite for End
        Given the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b o   | ab,bc |

    Scenario: Testbot - Test on oneway segment, Start End opposite side, option both for Start and End, 
        Given the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b b   | ab,bc |

    Scenario: Testbot - Test on oneway segment, Start End opposite side, option both for Start and Default for End
        Given the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b d   | ab,bc |

    Scenario: Testbot - Test on oneway segment, Start End opposite side, option both for Start and Opposite for End
        Given the node map
            """
               s
            a------b------c
                        e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |

        When I route I should get
            | from | to | sides | route |
            | s    | e  | b o   | ab,bc |




    Scenario: Testbot - UTurn test, router can't found a route because uturn unauthorized on the segment selected
        Given the node map
            """
               s        e
            a------b------c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

            
        And the relations
            | type        | way:from | way:to | node:via | restriction |
            | restriction | bc       | bc     | c        | no_u_turn   |

        When I route I should get
            | from | to | sides | route    |
            | s    | e  | b d   |          |













        