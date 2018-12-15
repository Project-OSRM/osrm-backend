@matrix @testbot
Feature: Basic Duration Matrix
# note that results of travel time are in seconds

    Background:
        Given the profile "testbot"
        And the partition extra arguments "--small-component-size 1 --max-cell-sizes 2,4,8,16"

    Scenario: Testbot - Travel time matrix of minimal network
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I request a travel time matrix I should get
            |   | a  | b  |
            | a | 0  | 10 |
            | b | 10 | 0  |

    @ch
    Scenario: Testbot - Travel time matrix of minimal network with toll exclude
        Given the query options
            | exclude  | toll        |

        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes | highway  | toll | #                                        |
            | ab    | motorway |      | not drivable for exclude=motorway        |
            | cd    | primary  |      | always drivable                          |
            | ac    | motorway | yes  | not drivable for exclude=toll and exclude=motorway,toll |
            | bd    | motorway | yes  | not drivable for exclude=toll and exclude=motorway,toll |

        When I request a travel time matrix I should get
            |   | a  | b  | c  | d  |
            | a | 0  | 15 |    |    |
            | b | 15 | 0  |    |    |
            | c |    |    | 0  | 10 |
            | d |    |    | 10 | 0  |

    @ch
    Scenario: Testbot - Travel time matrix of minimal network with motorway exclude
        Given the query options
            | exclude  | motorway  |

        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes | highway     | #                                 |
            | ab    | motorway    | not drivable for exclude=motorway |
            | cd    | residential |                                   |
            | ac    | residential |                                   |
            | bd    | residential |                                   |

        When I request a travel time matrix I should get
            |   | a | b  | c  | d  |
            | a | 0 | 45 | 15 | 30 |

    @ch
    Scenario: Testbot - Travel time matrix of minimal network disconnected motorway exclude
        Given the query options
            | exclude  | motorway  |

        Given the node map
            """
            ab                  efgh
            cd
            """

        And the ways
            | nodes | highway     | #                                 |
            | be    | motorway    | not drivable for exclude=motorway |
            | abcd  | residential |                                   |
            | efgh  | residential |                                   |

        When I request a travel time matrix I should get
            |   | a | b   | e |
            | a | 0 | 7.5 |   |

    @ch
    Scenario: Testbot - Travel time matrix of minimal network with motorway and toll excludes
        Given the query options
            | exclude  | motorway,toll  |

        Given the node map
            """
            a b          e f
            c d          g h
            """

        And the ways
            | nodes | highway     | toll | #                                 |
            | be    | motorway    |      | not drivable for exclude=motorway |
            | dg    | primary     | yes  | not drivable for exclude=toll     |
            | abcd  | residential |      |                                   |
            | efgh  | residential |      |                                   |

        When I request a travel time matrix I should get
            |   | a | b  | e | g |
            | a | 0 | 15 |   |   |

    Scenario: Testbot - Travel time matrix with different way speeds
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway   |
            | ab    | primary   |
            | bc    | secondary |
            | cd    | tertiary  |

        When I request a travel time matrix I should get
            |   | a  | b  | c  | d  |
            | a | 0  | 10 | 30 | 60 |
            | b | 10 | 0  | 20 | 50 |
            | c | 30 | 20 | 0  | 30 |
            | d | 60 | 50 | 30 | 0  |

        When I request a travel time matrix I should get
            |   | a  | b  | c  | d  |
            | a | 0  | 10 | 30 | 60 |

        When I request a travel time matrix I should get
            |   |  a |
            | a |  0 |
            | b | 10 |
            | c | 30 |
            | d | 60 |


    Scenario: Testbot - Travel time matrix of small grid
        Given the node map
            """
            a b c
            d e f
            """

        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |

        When I request a travel time matrix I should get
            |   | a  | b  | e  | f  |
            | a | 0  | 10 | 20 | 30 |
            | b | 10 | 0  | 10 | 20 |
            | e | 20 | 10 | 0  | 10 |
            | f | 30 | 20 | 10 | 0  |


    Scenario: Testbot - Travel time matrix of network with unroutable parts
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |

        When I request a travel time matrix I should get
            |   | a | b  |
            | a | 0 | 10 |
            | b |   | 0  |


    Scenario: Testbot - Travel time matrix of network with oneways
        Given the node map
            """
            x a b y
              d e
            """

        And the ways
            | nodes | oneway |
            | abeda | yes    |
            | xa    |        |
            | by    |        |

        When I request a travel time matrix I should get
            |   | x  | y   | d  | e  |
            | x | 0  | 30  | 40 | 30 |
            | y | 50 | 0   | 30 | 20 |
            | d | 20 | 30  | 0  | 30 |
            | e | 30 | 40  | 10 | 0  |


    Scenario: Testbot - Rectangular travel time matrix
        Given the node map
            """
            a b c
            d e f
            """

        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |

        When I request a travel time matrix I should get
            |   | a | b  | e  | f  |
            | a | 0 | 10 | 20 | 30 |

        When I request a travel time matrix I should get
            |   |  a |
            | a |  0 |
            | b | 10 |
            | e | 20 |
            | f | 30 |

        When I request a travel time matrix I should get
            |   |  a |  b |  e |  f |
            | a |  0 | 10 | 20 | 30 |
            | b | 10 |  0 | 10 | 20 |

        When I request a travel time matrix I should get
            |   |  a |  b |
            | a |  0 | 10 |
            | b | 10 |  0 |
            | e | 20 | 10 |
            | f | 30 | 20 |

        When I request a travel time matrix I should get
            |   |  a |  b |  e |  f |
            | a |  0 | 10 | 20 | 30 |
            | b | 10 |  0 | 10 | 20 |
            | e | 20 | 10 |  0 | 10 |

        When I request a travel time matrix I should get
            |   |  a |  b |  e |
            | a |  0 | 10 | 20 |
            | b | 10 |  0 | 10 |
            | e | 20 | 10 |  0 |
            | f | 30 | 20 | 10 |

        When I request a travel time matrix I should get
            |   |  a |  b |  e |  f |
            | a |  0 | 10 | 20 | 30 |
            | b | 10 |  0 | 10 | 20 |
            | e | 20 | 10 |  0 | 10 |
            | f | 30 | 20 | 10 |  0 |

     Scenario: Testbot - Travel time 3x2 matrix
        Given the node map
            """
            a b c
            d e f
            """

        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |

        When I request a travel time matrix I should get
            |   | b  | e  | f  |
            | a | 10 | 20 | 30 |
            | b | 0  | 10 | 20 |


    Scenario: Testbot - All coordinates are from same small component
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the node map
            """
            a b   f
            d e   g
            """

        And the ways
            | nodes |
            | ab    |
            | be    |
            | ed    |
            | da    |
            | fg    |

        When I request a travel time matrix I should get
            |   | f  | g  |
            | f | 0  | 30 |
            | g | 30 |  0 |


    Scenario: Testbot - Coordinates are from different small component and snap to big CC
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the node map
            """
            a b   f h
            d e   g i
            """

        And the ways
            | nodes |
            | ab    |
            | be    |
            | ed    |
            | da    |
            | fg    |
            | hi    |

        When I request a travel time matrix I should get
            |   | f  | g  | h  | i  |
            | f | 0  | 30 | 0  | 30 |
            | g | 30 |  0 | 30 | 0  |
            | h | 0  | 30 | 0  | 30 |
            | i | 30 |  0 | 30 | 0  |


    Scenario: Testbot - Travel time matrix with loops
        Given the node map
            """
            a 1 2 b
            d 4 3 c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes |
            | bc    | yes |
            | cd    | yes |
            | da    | yes |

        When I request a travel time matrix I should get
            |   | 1      | 2      | 3      | 4      |
            | 1 | 0      | 10 +-1 | 40 +-1 | 50 +-1 |
            | 2 | 70 +-1 | 0      | 30 +-1 | 40 +-1 |
            | 3 | 40 +-1 | 50 +-1 | 0      | 10 +-1 |
            | 4 | 30 +-1 | 40 +-1 | 70 +-1 | 0      |


    Scenario: Testbot - Travel time matrix based on segment durations
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.traffic_signal_penalty = 0
          profile.u_turn_penalty = 0
          return profile
        end

        functions.process_segment = function(profile, segment)
          segment.weight = 2
          segment.duration = 11
        end

        return functions
        """

        And the node map
          """
          a-b-c-d
              .
              e
          """

        And the ways
          | nodes |
          | abcd  |
          | ce    |

        When I request a travel time matrix I should get
          |   |  a |  b |  c |  d |  e |
          | a |  0 | 11 | 22 | 33 | 33 |
          | b | 11 |  0 | 11 | 22 | 22 |
          | c | 22 | 11 |  0 | 11 | 11 |
          | d | 33 | 22 | 11 |  0 | 22 |
          | e | 33 | 22 | 11 | 22 |  0 |


    Scenario: Testbot - Travel time matrix for alternative loop paths
        Given the profile file
        """
        local functions = require('testbot')
        functions.setup_testbot = functions.setup

        functions.setup = function()
          local profile = functions.setup_testbot()
          profile.traffic_signal_penalty = 0
          profile.u_turn_penalty = 0
          profile.weight_precision = 3
          return profile
        end

        functions.process_segment = function(profile, segment)
          segment.weight = 777
          segment.duration = 3
        end

        return functions
        """
        And the node map
            """
            a 2 1 b
            7     4
            8     3
            c 5 6 d
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bd    | yes    |
            | dc    | yes    |
            | ca    | yes    |

        When I request a travel time matrix I should get
          |   |   1 |   2 |   3 |   4 |    5 |   6 |   7 |   8 |
          | 1 |   0 |  11 |   3 |   2 |    6 |   5 | 8.9 | 7.9 |
          | 2 |   1 |   0 |   4 |   3 |    7 |   6 | 9.9 | 8.9 |
          | 3 |   9 |   8 |   0 |  11 |    3 |   2 | 5.9 | 4.9 |
          | 4 |  10 |   9 |   1 |   0 |    4 |   3 | 6.9 | 5.9 |
          | 5 |   6 |   5 |   9 |   8 |    0 |  11 | 2.9 | 1.9 |
          | 6 |   7 |   6 |  10 |   9 |    1 |   0 | 3.9 | 2.9 |
          | 7 | 3.1 | 2.1 | 6.1 | 5.1 |  9.1 | 8.1 |   0 |  11 |
          | 8 | 4.1 | 3.1 | 7.1 | 6.1 | 10.1 | 9.1 |   1 | 0   |


    Scenario: Testbot - Travel time matrix with ties
        Given the profile file
        """
        local functions = require('testbot')
        functions.process_segment = function(profile, segment)
          segment.weight = 1
          segment.duration = 1
        end
        functions.process_turn = function(profile, turn)
          if turn.angle >= 0 then
            turn.duration = 16
          else
            turn.duration = 4
          end
          turn.weight = 0
        end
        return functions
        """
        And the node map
            """
            a     b

            c     d
            """

        And the ways
          | nodes |
          | ab    |
          | ac    |
          | bd    |
          | dc    |

        When I route I should get
          | from | to | route | distance | time | weight |
          | a    | c  | ac,ac | 200m     | 5s   |      5 |

        When I request a travel time matrix I should get
          |   | a | b | c |  d |
          | a | 0 | 1 | 5 | 10 |

        When I request a travel time matrix I should get
          |   |  a |
          | a |  0 |
          | b |  1 |
          | c | 15 |
          | d | 10 |

    Scenario: Testbot - OneToMany vs ManyToOne
        Given the node map
            """
            a b
            c
            """

        And the ways
            | nodes  | oneway |
            | ab     | yes    |
            | ac     |        |
            | bc     |        |

        When I request a travel time matrix I should get
            |   |   a  |  b  |
            | b | 24.1 |  0  |

        When I request a travel time matrix I should get
            |   |   a   |
            | a |   0   |
            | b | 24.1  |

    Scenario: Testbot - Filling in noroutes with estimates (defaults to input coordinate location)
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the query options
            | fallback_speed | 5 |
        Given the node map
            """
            a b   f h 1
            d e   g i
            """

        And the ways
            | nodes |
            | abeda |
            | fhigf |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 30 | 18 | 30 |
            | b | 30 | 0  | 12 | 24 |
            | f | 18 | 12 | 0  | 30 |
            | 1 | 30 | 24 | 30 | 0  |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 30 | 18 | 30 |

        When I request a travel time matrix I should get
            |   | a  |
            | a | 0  |
            | b | 30 |
            | f | 18 |
            | 1 | 30 |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |
            | b |   |   | Y | Y |
            | f | Y | Y |   |   |
            | 1 | Y | Y |   |   |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |

        When I request a travel time matrix I should get estimates for
            |   | a |
            | a |   |
            | b |   |
            | f | Y |
            | 1 | Y |

    Scenario: Testbot - Filling in noroutes with estimates - use input coordinate
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the query options
            | fallback_speed | 5 |
            | fallback_coordinate | input |
        Given the node map
            """
            a b   f h 1
            d e   g i
            """

        And the ways
            | nodes |
            | abeda |
            | fhigf |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 30 | 18 | 30 |
            | b | 30 | 0  | 12 | 24 |
            | f | 18 | 12 | 0  | 30 |
            | 1 | 30 | 24 | 30 | 0  |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 30 | 18 | 30 |

        When I request a travel time matrix I should get
            |   | a  |
            | a | 0  |
            | b | 30 |
            | f | 18 |
            | 1 | 30 |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |
            | b |   |   | Y | Y |
            | f | Y | Y |   |   |
            | 1 | Y | Y |   |   |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |

        When I request a travel time matrix I should get estimates for
            |   | a |
            | a |   |
            | b |   |
            | f | Y |
            | 1 | Y |


    Scenario: Testbot - Filling in noroutes with estimates - use snapped coordinate
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the query options
            | fallback_speed | 5 |
            | fallback_coordinate | snapped |
        Given the node map
            """
            a b   f h 1
            d e   g i
            """

        And the ways
            | nodes |
            | abeda |
            | fhigf |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 30 | 18 | 24 |
            | b | 30 | 0  | 12 | 18 |
            | f | 18 | 12 | 0  | 30 |
            | 1 | 24 | 18 | 30 | 0  |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 30 | 18 | 24 |

        When I request a travel time matrix I should get
            |   | a  |
            | a | 0  |
            | b | 30 |
            | f | 18 |
            | 1 | 24 |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |
            | b |   |   | Y | Y |
            | f | Y | Y |   |   |
            | 1 | Y | Y |   |   |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |

        When I request a travel time matrix I should get estimates for
            |   | a |
            | a |   |
            | b |   |
            | f | Y |
            | 1 | Y |

    Scenario: Testbot - Travel time matrix of minimal network with scale factor
        Given the query options
            | scale_factor | 2 |
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        When I request a travel time matrix I should get
            |   | a  | b  |
            | a | 0  | 20 |
            | b | 20 | 0  |
     Scenario: Testbot - Test fallback speeds and scale factor
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the query options
            | scale_factor | 2 |
            | fallback_speed | 5 |
            | fallback_coordinate | snapped |

        Given the node map
            """
            a b   f h 1
            d e   g i
            """

        And the ways
            | nodes |
            | abeda |
            | fhigf |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 60 | 36 | 48 |
            | b | 60 | 0  | 24 | 36 |
            | f | 36 | 24 | 0  | 60 |
            | 1 | 48 | 36 | 60 | 0  |

        When I request a travel time matrix I should get
            |   | a  | b  | f  | 1  |
            | a | 0  | 60 | 36 | 48 |

        When I request a travel time matrix I should get
            |   | a  |
            | a | 0  |
            | b | 60 |
            | f | 36 |
            | 1 | 48 |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |
            | b |   |   | Y | Y |
            | f | Y | Y |   |   |
            | 1 | Y | Y |   |   |

        When I request a travel time matrix I should get estimates for
            |   | a | b | f | 1 |
            | a |   |   | Y | Y |

        When I request a travel time matrix I should get estimates for
            |   | a |
            | a |   |
            | b |   |
            | f | Y |
            | 1 | Y |


     Scenario: Testbot - Travel time matrix of minimal network with overflow scale factor
        Given the query options
            | scale_factor | 2147483647 |

        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I request a travel time matrix I should get
            |   | a           | b           |
            | a | 0           | 214748364.6 |
            | b | 214748364.6 | 0           |

     Scenario: Testbot - Travel time matrix of minimal network with fraction scale factor
        Given the query options
            | scale_factor | 0.5 |

        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I request a travel time matrix I should get
            |   | a  | b  |
            | a | 0  | 5  |
            | b | 5  | 0  |
