@routing @testbot @mode
Feature: Testbot - Travel mode

# testbot modes:
# 1 driving
# 2 route
# 3 river downstream
# 4 river upstream
# 5 steps down
# 6 steps up

    Background:
       Given the profile "testbot"
       Given a grid size of 200 meters

    Scenario: Testbot - Always announce mode change
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway     | name |
            | ab    | residential | foo  |
            | bc    | river       | foo  |
            | cd    | residential | foo  |

        When I route I should get
            | from | to | route           | modes                                    |
            | a    | d  | foo,foo,foo,foo | driving,river downstream,driving,driving |
            | b    | d  | foo,foo,foo     | river downstream,driving,driving         |

    Scenario: Testbot - Compressed Modes
        Given the node map
            """
            a b c d e f g
            """

        And the ways
            | nodes | highway     | name   |
            | abc   | residential | road   |
            | cde   | river       | liquid |
            | efg   | residential | solid  |

        When I route I should get
            | from | to | route                    | modes                                    |
            | a    | g  | road,liquid,solid,solid  | driving,river downstream,driving,driving |
            | c    | g  | liquid,solid,solid       | river downstream,driving,driving         |

    Scenario: Testbot - Modes in each direction, different forward/backward speeds
        Given the node map
            """
              0 1
            a     b
            """

        And the ways
            | nodes | highway | oneway |
            | ab    | river   |        |

        When I route I should get
            | from | to | route | modes                             |
            | a    | 0  | ab,ab | river downstream,river downstream |
            | a    | b  | ab,ab | river downstream,river downstream |
            | 0    | 1  | ab,ab | river downstream,river downstream |
            | 0    | b  | ab,ab | river downstream,river downstream |
            | b    | 1  | ab,ab | river upstream,river upstream     |
            | b    | a  | ab,ab | river upstream,river upstream     |
            | 1    | 0  | ab,ab | river upstream,river upstream     |
            | 1    | a  | ab,ab | river upstream,river upstream     |

    Scenario: Testbot - Modes in each direction, same forward/backward speeds
        Given the node map
            """
              0 1
            a     b
            """

        And the ways
            | nodes | highway |
            | ab    | steps   |

        When I route I should get
            | from | to | route | modes                 | time     |
            | 0    | 1  | ab,ab | steps down,steps down | 120s +-1 |
            | 1    | 0  | ab,ab | steps up,steps up     | 120s +-1 |

    @oneway
    Scenario: Testbot - Modes for oneway, different forward/backward speeds
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | oneway |
            | ab    | river   | yes    |

        When I route I should get
            | from | to | route | modes                             |
            | a    | b  | ab,ab | river downstream,river downstream |
            | b    | a  |       |                                   |

    @oneway
    Scenario: Testbot - Modes for oneway, same forward/backward speeds
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | oneway |
            | ab    | steps   | yes    |

        When I route I should get
            | from | to | route | modes                 |
            | a    | b  | ab,ab | steps down,steps down |
            | b    | a  |       |                       |

    @oneway
    Scenario: Testbot - Modes for reverse oneway, different forward/backward speeds
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | oneway |
            | ab    | river   | -1     |

        When I route I should get
            | from | to | route | modes                         |
            | a    | b  |       |                               |
            | b    | a  | ab,ab | river upstream,river upstream |

    @oneway
    Scenario: Testbot - Modes for reverse oneway, same forward/backward speeds
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway | oneway |
            | ab    | steps   | -1     |

        When I route I should get
            | from | to | route | modes             |
            | a    | b  |       |                   |
            | b    | a  | ab,ab | steps up,steps up |

    @via
    Scenario: Testbot - Mode should be set at via points
        Given the node map
            """
            a 1 b
            """

        And the ways
            | nodes | highway |
            | ab    | river   |

        When I route I should get
            | waypoints | route       | modes                                                               |
            | a,1,b     | ab,ab,ab,ab | river downstream,river downstream,river downstream,river downstream |
            | b,1,a     | ab,ab,ab,ab | river upstream,river upstream,river upstream,river upstream         |

    Scenario: Testbot - Starting at a tricky node
       Given the node map
            """
              a
                  b c
            """

       And the ways
            | nodes | highway |
            | ab    | river   |
            | bc    | primary |

       When I route I should get
            | from | to | route | modes                         |
            | b    | a  | ab,ab | river upstream,river upstream |

    Scenario: Testbot - Mode changes on straight way without name change
       Given the node map
            """
            a 1 b 2 c
            """

       And the ways
            | nodes | highway | name   |
            | ab    | primary | Avenue |
            | bc    | river   | Avenue |

       When I route I should get
            | from | to | route                | modes                                     |
            | a    | c  | Avenue,Avenue,Avenue | driving,river downstream,river downstream |
            | c    | a  | Avenue,Avenue,Avenue | river upstream,driving,driving            |
            | 1    | 2  | Avenue,Avenue,Avenue | driving,river downstream,river downstream |
            | 2    | 1  | Avenue,Avenue,Avenue | river upstream,driving,driving            |

    Scenario: Testbot - Mode for routes
       Given the node map
            """
            a b
              c d e f
            """

       And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |
            | de    | primary |       |          |
            | ef    | primary |       |          |

       When I route I should get
            | from | to | route             | modes                                         |
            | a    | d  | ab,bc,cd,cd       | driving,route,driving,driving                 |
            | d    | a  | cd,bc,ab,ab       | driving,route,driving,driving                 |
            | c    | a  | bc,ab,ab          | route,driving,driving                         |
            | d    | b  | cd,bc,bc          | driving,route,route                           |
            | a    | c  | ab,bc,bc          | driving,route,route                           |
            | b    | d  | bc,cd,cd          | route,driving,driving                         |
            | a    | f  | ab,bc,cd,de,ef,ef | driving,route,driving,driving,driving,driving |

    Scenario: Testbot - Modes, triangle map
        Given the node map
            """
                        d
                      2
                    6   5
            a 0 b c
                    4   1
                      3
                        e
            """

       And the ways
            | nodes | highway | oneway |
            | abc   | primary |        |
            | cd    | primary | yes    |
            | ce    | river   |        |
            | de    | primary |        |

       When I route I should get
            | from | to | route            | modes                                          |
            | 0    | 1  | abc,ce,de,de     | driving,river downstream,driving,driving       |
            | 1    | 0  | de,ce,abc,abc    | driving,river upstream,driving,driving         |
            | 0    | 2  | abc,cd,cd        | driving,driving,driving                        |
            | 2    | 0  | cd,de,ce,abc,abc | driving,driving,river upstream,driving,driving |
            | 0    | 3  | abc,ce,ce        | driving,river downstream,river downstream      |
            | 3    | 0  | ce,abc,abc       | river upstream,driving,driving                 |
            | 4    | 3  | ce,ce            | river downstream,river downstream              |
            | 3    | 4  | ce,ce            | river upstream,river upstream                  |
            | 3    | 1  | ce,de,de         | river downstream,driving,driving               |
            | 1    | 3  | de,ce,ce         | driving,river upstream,river upstream          |
            | a    | e  | abc,ce,ce        | driving,river downstream,river downstream      |
            | e    | a  | ce,abc,abc       | river upstream,driving,driving                 |
            | a    | d  | abc,cd,cd        | driving,driving,driving                        |
            | d    | a  | de,ce,abc,abc    | driving,river upstream,driving,driving         |

    Scenario: Testbot - River in the middle
        Given the node map
            """
            a b c
                d
                e f g
            """

        And the ways
            | nodes | highway |
            | abc   | primary |
            | cde   | river   |
            | efg   | primary |

        When I route I should get
            | from | to | route           | modes                                    |
            | a    | g  | abc,cde,efg,efg | driving,river downstream,driving,driving |
            | b    | f  | abc,cde,efg,efg | driving,river downstream,driving,driving |
            | e    | c  | cde,cde         | river upstream,river upstream            |
            | e    | b  | cde,abc,abc     | river upstream,driving,driving           |
            | e    | a  | cde,abc,abc     | river upstream,driving,driving           |
            | c    | e  | cde,cde         | river downstream,river downstream        |
            | c    | f  | cde,efg,efg     | river downstream,driving,driving         |
            | c    | g  | cde,efg,efg     | river downstream,driving,driving         |
