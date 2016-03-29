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

    @mokob @2166
    Scenario: Testbot - Always announce mode change
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway     | name |
            | ab    | residential | foo  |
            | bc    | river       | foo  |
            | cd    | residential | foo  |

        When I route I should get
            | from | to | route        | modes                            |
            | a    | d  | foo,foo,foo  | driving,river downstream,driving |
            | b    | d  | foo,foo      | river downstream,driving         |

    @mokob @2166
    Scenario: Testbot - Compressed Modes
        Given the node map
            | a | b | c | d | e | f | g |

        And the ways
            | nodes | highway     | name   |
            | abc   | residential | road   |
            | cde   | river       | liquid |
            | efg   | residential | solid  |

        When I route I should get
            | from | to | route              | modes                            | turns                           |
            | a    | g  | road,liquid,solid  | driving,river downstream,driving | depart,straight,straight,arrive |
            | c    | g  | liquid,solid       | river downstream,driving         | depart,straight,arrive          |

    @mokob @2166
    Scenario: Testbot - Modes in each direction, different forward/backward speeds
        Given the node map
            |   | 0 | 1 |   |
            | a |   |   | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | river   |        |

        When I route I should get
            | from | to | route | modes            |
            | a    | 0  | ab    | river downstream |
            | a    | b  | ab    | river downstream |
            | 0    | 1  | ab    | river downstream |
            | 0    | b  | ab    | river downstream |
            | b    | 1  | ab    | river upstream   |
            | b    | a  | ab    | river upstream   |
            | 1    | 0  | ab    | river upstream   |
            | 1    | a  | ab    | river upstream   |

    Scenario: Testbot - Modes in each direction, same forward/backward speeds
        Given the node map
            |   | 0 | 1 |   |
            | a |   |   | b |

        And the ways
            | nodes | highway |
            | ab    | steps   |

        When I route I should get
            | from | to | route | modes      | time    |
            | 0    | 1  | ab    | steps down | 60s +-1 |
            | 1    | 0  | ab    | steps up   | 60s +-1 |

    @oneway @mokob @2166
    Scenario: Testbot - Modes for oneway, different forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | river   | yes    |

        When I route I should get
            | from | to | route | modes            |
            | a    | b  | ab    | river downstream |
            | b    | a  |       |                  |

    @oneway
    Scenario: Testbot - Modes for oneway, same forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | steps   | yes    |

        When I route I should get
            | from | to | route | modes      |
            | a    | b  | ab    | steps down |
            | b    | a  |       |            |

    @oneway @mokob @2166
    Scenario: Testbot - Modes for reverse oneway, different forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | river   | -1     |

        When I route I should get
            | from | to | route | modes          |
            | a    | b  |       |                |
            | b    | a  | ab    | river upstream |

    @oneway
    Scenario: Testbot - Modes for reverse oneway, same forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | steps   | -1     |

        When I route I should get
            | from | to | route | modes    |
            | a    | b  |       |          |
            | b    | a  | ab    | steps up |

    @via @mokob @2166
    Scenario: Testbot - Mode should be set at via points
        Given the node map
            | a | 1 | b |

        And the ways
            | nodes | highway |
            | ab    | river   |

        When I route I should get
            | waypoints | route | modes                             | turns             |
            | a,1,b     | ab,ab | river downstream,river downstream | depart,via,arrive |
            | b,1,a     | ab,ab | river upstream,river upstream     | depart,via,arrive |

    @mokob @2166
    Scenario: Testbot - Starting at a tricky node
       Given the node map
            |  | a |  |   |   |
            |  |   |  | b | c |

       And the ways
            | nodes | highway |
            | ab    | river   |
            | bc    | primary |

       When I route I should get
            | from | to | route | modes          |
            | b    | a  | ab    | river upstream |

    @mokob @2166
    Scenario: Testbot - Mode changes on straight way without name change
       Given the node map
            | a | 1 | b | 2 | c |

       And the ways
            | nodes | highway | name   |
            | ab    | primary | Avenue |
            | bc    | river   | Avenue |

       When I route I should get
            | from | to | route         | modes                    | turns                  |
            | a    | c  | Avenue,Avenue | driving,river downstream | depart,straight,arrive |
            | c    | a  | Avenue,Avenue | river upstream,driving   | depart,straight,arrive |
            | 1    | 2  | Avenue,Avenue | driving,river downstream | depart,straight,arrive |
            | 2    | 1  | Avenue,Avenue | river upstream,driving   | depart,straight,arrive |

    Scenario: Testbot - Mode for routes
       Given the node map
            | a | b |   |   |   |
            |   | c | d | e | f |

       And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |
            | de    | primary |       |          |
            | ef    | primary |       |          |

       When I route I should get
            | from | to | route          | turns                                              | modes                                 |
            | a    | d  | ab,bc,cd       | depart,right,left,arrive                           | driving,route,driving                 |
            | d    | a  | cd,bc,ab       | depart,right,left,arrive                           | driving,route,driving                 |
            | c    | a  | bc,ab          | depart,left,arrive                                 | route,driving                         |
            | d    | b  | cd,bc          | depart,right,arrive                                | driving,route                         |
            | a    | c  | ab,bc          | depart,right,arrive                                | driving,route                         |
            | b    | d  | bc,cd          | depart,left,arrive                                 | route,driving                         |
            | a    | f  | ab,bc,cd,de,ef | depart,right,left,straight,straight,arrive         | driving,route,driving,driving,driving |

    @mokob @2166
    Scenario: Testbot - Modes, triangle map
        Given the node map
            |   |   |   |   |   |   | d |
            |   |   |   |   |   | 2 |   |
            |   |   |   |   | 6 |   | 5 |
            | a | 0 | b | c |   |   |   |
            |   |   |   |   | 4 |   | 1 |
            |   |   |   |   |   | 3 |   |
            |   |   |   |   |   |   | e |

       And the ways
            | nodes | highway | oneway |
            | abc   | primary |        |
            | cd    | primary | yes    |
            | ce    | river   |        |
            | de    | primary |        |

       When I route I should get
            | from | to | route        | modes   |
            | 0    | 1  | abc,ce,de    | driving,river downstream,driving       |
            | 1    | 0  | de,ce,abc    | driving,river upstream,driving         |
            | 0    | 2  | abc,cd       | driving,driving                        |
            | 2    | 0  | cd,de,ce,abc | driving,driving,river upstream,driving |
            | 0    | 3  | abc,ce       | driving,river downstream               |
            | 3    | 0  | ce,abc       | river upstream,driving                 |
            | 4    | 3  | ce           | river downstream                       |
            | 3    | 4  | ce           | river upstream                         |
            | 3    | 1  | ce,de        | river downstream,driving               |
            | 1    | 3  | de,ce        | driving,river upstream                 |
            | a    | e  | abc,ce       | driving,river downstream               |
            | e    | a  | ce,abc       | river upstream,driving                 |
            | a    | d  | abc,cd       | driving,driving                        |
            | d    | a  | de,ce,abc    | driving,river upstream,driving         |

    @mokob @2166
    Scenario: Testbot - River in the middle
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | cde   | river   |
            | efg   | primary |

        When I route I should get
            | from | to | route       | modes                            |
            | a    | g  | abc,cde,efg | driving,river downstream,driving |
            | b    | f  | abc,cde,efg | driving,river downstream,driving |
            | e    | c  | cde         | river upstream                   |
            | e    | b  | cde,abc     | river upstream,driving           |
            | e    | a  | cde,abc     | river upstream,driving           |
            | c    | e  | cde         | river downstream                 |
            | c    | f  | cde,efg     | river downstream,driving         |
            | c    | g  | cde,efg     | river downstream,driving         |
