@matrix @testbot
Feature: Basic Distance Matrix
# note that results of travel distance are in metres

    Background:
        Given the profile "testbot"
        And the partition extra arguments "--small-component-size 1 --max-cell-sizes 2,4,8,16"
  Scenario: Testbot - Travel distance matrix of small grid
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

        When I request a travel distance matrix I should get
            |   |    a    |    b    |     e   |   f    |
            | a |    0    |  100    |  199.9  |  300   |
            | b |  100    |   0     |  100    |  200   |
            | e |  199.9  |  100    |   0     |  100.1 |
            | f |  300    |  200    |  100.1  |   0    |

    Scenario: Testbot - Travel distance matrix of minimal network exact distances
        Given the node map
            """
            a z
              b
              c
              d
            """

        And the ways
            | nodes |
            | az    |
            | zbcd  |

        When I request a travel distance matrix I should get
            |   |   a    |   z     |   b     |   c     |   d     |
            | a |   0    |  100    |  199.9  |  300    |  399.9  |
            | z |  100   |  0      |  100    |  200    |  300    |
            | b |  199.9 |  100    |  0      |  100.1  |  200    |
            | c |  300   |  200    |  100.1  |  0      |  100    |
            | d |  399.9 |  300    |  200    |  100    |  0      |

    Scenario: Testbot - Travel distance matrix of minimal network with toll exclude
        Given the query options
            | exclude  | toll        |

        Given the node map
            """
            a b
            c d
            """

        And the ways
            | nodes | highway  | toll | #                                                       |
            | ab    | motorway |      | not drivable for exclude=motorway                       |
            | cd    | primary  |      | always drivable                                         |
            | ac    | primary  | yes  | not drivable for exclude=toll and exclude=motorway,toll |
            | bd    | motorway | yes  | not drivable for exclude=toll and exclude=motorway,toll |

        When I request a travel distance matrix I should get
            |   | a     | b     | c     | d     |
            | a | 0     | 100   |       |       |
            | b | 100   | 0     |       |       |
            | c |       |       | 0     | 100   |
            | d |       |       | 100   | 0     |

    Scenario: Testbot - Travel distance matrix of minimal network with motorway exclude
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

        When I request a travel distance matrix I should get
            |   | a |   b    |  c   |   d   |
            | a | 0 |  299.9 | 100  | 199.9 |

    Scenario: Testbot - Travel distance matrix of minimal network disconnected motorway exclude
        Given the query options
            | exclude  | motorway  |
        And the extract extra arguments "--small-component-size 4"

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

        When I request a travel distance matrix I should get
            |   | a | b    | e |
            | a | 0 | 50   |   |

    Scenario: Testbot - Travel distance matrix of minimal network with motorway and toll excludes
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

        When I request a travel distance matrix I should get
            |   | a | b      | e | g |
            | a | 0 | 100    |   |   |

    Scenario: Testbot - Travel distance matrix with different way speeds
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway   |
            | ab    | primary   |
            | bc    | secondary |
            | cd    | tertiary  |

        When I request a travel distance matrix I should get
            |   | a     | b     | c     | d     |
            | a | 0     | 100   | 200   | 300   |
            | b | 100   | 0     | 100.1 | 200   |
            | c | 200   | 100.1 | 0     | 100   |
            | d | 300   | 200   | 100   | 0     |

        When I request a travel distance matrix I should get
            |   | a | b     | c     | d     |
            | a | 0 | 100   | 200   | 300   |

        When I request a travel distance matrix I should get
            |   | a     |
            | a | 0     |
            | b | 100   |
            | c | 200   |
            | d | 300   |

    Scenario: Testbot - Travel distance matrix of small grid
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

        When I request a travel distance matrix I should get
            |   | a     | b     | e     | f     |
            | a | 0     | 100   | 199.9 | 300   |
            | b | 100   | 0     | 100   | 200   |
            | e | 199.9 | 100   | 0     | 100.1 |
            | f | 300   | 200   | 100.1 | 0     |

    Scenario: Testbot - Travel distance matrix of network with unroutable parts
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |

        When I request a travel distance matrix I should get
            |   | a | b      |
            | a | 0 | 100    |
            | b |   | 0      |

    Scenario: Testbot - Travel distance matrix of network with oneways
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

        When I request a travel distance matrix I should get
            |   | x     | y     | d     | e     |
            | x | 0     | 300   | 400   | 300   |
            | y | 499.9 | 0     | 300   | 199.9 |
            | d | 199.9 | 300   | 0     | 300   |
            | e | 300   | 400   | 100.1 | 0     |

    Scenario: Testbot - Rectangular travel distance matrix
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

        When I route I should get
            | from | to | distance  |
            | e    | a  | 199.9m    |
            | e    | b  | 100m      |
            | f    | a  | 300m      |
            | f    | b  | 200m      |

        When I request a travel distance matrix I should get
            |   | a | b      | e      | f      |
            | a | 0 |  100   | 199.9  |  300   |

        When I request a travel distance matrix I should get
            |   | a     |
            | a | 0     |
            | b | 100   |
            | e | 199.9 |
            | f | 300   |

        When I request a travel distance matrix I should get
            |   | a     | b     | e     | f     |
            | a | 0     | 100   | 199.9 | 300   |
            | b | 100   | 0     | 100   | 200   |

        When I request a travel distance matrix I should get
            |   | a     | b     |
            | a | 0     | 100   |
            | b | 100   | 0     |
            | e | 199.9 | 100   |
            | f | 300   | 200   |

        When I request a travel distance matrix I should get
            |   | a     | b     | e     | f     |
            | a | 0     | 100   | 199.9 | 300   |
            | b | 100   | 0     | 100   | 200   |
            | e | 199.9 | 100   | 0     | 100.1 |

        When I request a travel distance matrix I should get
             |   | a     | b     | e     |
             | a | 0     | 100   | 199.9 |
             | b | 100   | 0     | 100   |
             | e | 199.9 | 100   | 0     |
             | f | 300   | 200   | 100.1 |

        When I request a travel distance matrix I should get
             |   | a     | b     | e     | f     |
             | a | 0     | 100   | 199.9 | 300   |
             | b | 100   | 0     | 100   | 200   |
             | e | 199.9 | 100   | 0     | 100.1 |
             | f | 300   | 200   | 100.1 | 0     |

     Scenario: Testbot - Travel distance 3x2 matrix
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


        When I request a travel distance matrix I should get
            |   | b     | e     | f     |
            | a | 100   | 199.9 | 300   |
            | b | 0     | 100   | 200   |

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

        When I request a travel distance matrix I should get
            |   | f     | g     |
            | f | 0     | 300   |
            | g | 300   | 0     |

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

        When I route I should get
            | from | to | distance |
            | f    | g  | 300m     |
            | f    | i  | 300m     |
            | g    | f  | 300m     |
            | g    | h  | 300m     |
            | h    | g  | 300m     |
            | h    | i  | 300m     |
            | i    | f  | 300m     |
            | i    | h  | 300m     |

        When I request a travel distance matrix I should get
            |   | f     | g     | h     | i     |
            | f | 0     | 300   | 0     | 300   |
            | g | 300   | 0     | 300   | 0     |
            | h | 0     | 300   | 0     | 300   |
            | i | 300   | 0     | 300   | 0     |

    Scenario: Testbot - Travel distance matrix with loops
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

        When I request a travel distance matrix I should get
            |   | 1     | 2     | 3     | 4     |
            | 1 | 0     | 100.1 | 399.9 | 500   |
            | 2 | 699.8 | 0     | 299.9 | 399.9 |
            | 3 | 399.9 | 500   | 0     | 100.1 |
            | 4 | 299.9 | 399.9 | 699.8 | 0     |


    Scenario: Testbot - Travel distance matrix based on segment durations
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

        When I request a travel distance matrix I should get
            |   | a     | b     | c     | d     | e     |
            | a | 0     | 100   | 200   | 300   | 400   |
            | b | 100   | 0     | 100.1 | 200   | 300.1 |
            | c | 200   | 100.1 | 0     | 100   | 200   |
            | d | 300   | 200   | 100   | 0     | 300   |
            | e | 400   | 300.1 | 200   | 300   | 0     |

    Scenario: Testbot - Travel distance matrix for alternative loop paths
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

        When I request a travel distance matrix I should get
             |   | 1     | 2      | 3     | 4      | 5     | 6      | 7     | 8      |
             | 1 | 0     | 1099.8 | 300   | 199.9  | 600   | 499.9  | 899.9 | 799.9  |
             | 2 | 100.1 | 0      | 400   | 300    | 700   | 600    | 1000  | 899.9  |
             | 3 | 899.9 | 799.9  | 0     | 1099.8 | 300   | 199.9  | 600   | 499.9  |
             | 4 | 1000  | 899.9  | 100.1 | 0      | 400   | 300    | 700   | 600    |
             | 5 | 600   | 499.9  | 899.9 | 799.9  | 0     | 1099.8 | 300   | 199.9  |
             | 6 | 700   | 600    | 1000  | 899.9  | 100.1 | 0      | 400   | 300    |
             | 7 | 300   | 199.9  | 600   | 499.9  | 899.9 | 799.9  | 0     | 1099.8 |
             | 8 | 400   | 300    | 700   | 600    | 1000  | 899.9  | 100.1 | 0      |

        When I request a travel distance matrix I should get
             |   | 1     |
             | 1 | 0     |
             | 2 | 100.1 |
             | 3 | 899.9 |
             | 4 | 1000  |
             | 5 | 600   |
             | 6 | 700   |
             | 7 | 300   |
             | 8 | 400   |

    Scenario: Testbot - Travel distance matrix with ties
        Given the node map
            """
            a        b

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
            | a    | c  | ac,ac | 200m     | 20s  |     20 |

        When I route I should get
            | from | to | route    | distance |
            | a    | b  | ab,ab    | 450m     |
            | a    | c  | ac,ac    | 200m     |
            | a    | d  | ac,dc,dc | 500m     |

        When I request a travel distance matrix I should get
            |   | a | b     | c     | d   |
            | a | 0 | 450   | 200   | 500 |

        When I request a travel distance matrix I should get
            |   | a     |
            | a | 0     |
            | b | 450   |
            | c | 200   |
            | d | 500   |

        When I request a travel distance matrix I should get
            |   | a     | c     |
            | a | 0     | 200   |
            | c | 200   | 0     |


    # Check rounding errors
    Scenario: Testbot - Long distances in tables
        Given a grid size of 1000 meters
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes    |
            | abcd     |

        When I request a travel distance matrix I should get
            |   | a | b      | c      | d      |
            | a | 0 | 1000.1 | 2000 | 3000.1 |


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

        When I request a travel distance matrix I should get
            |   |   a   | b      |
            | b | 241.3 | 0      |

        When I request a travel distance matrix I should get
            |   |   a   |
            | a |   0   |
            | b | 241.3 |

    Scenario: Testbot - Varying distances between nodes
        Given the node map
            """
            a b   c      d

            e



            f
            """

        And the ways
            | nodes  | oneway |
            | feabcd | yes    |
            | ec     |        |
            | fd     |        |

        When I request a travel distance matrix I should get
            |   | a      | b     | c      | d      | e      | f      |
            | a | 0      | 100   | 300    | 650    | 660.5  | 1534.6 |
            | b | 760.6  | 0     | 200    | 550.1  | 560.6  | 1434.6 |
            | c | 560.6  | 660.5 | 0      | 350    | 360.5  | 1234.6 |
            | d | 1484.6 | 1584.5| 1645.1 | 0      | 1284.5 | 884.6  |
            | e | 200    | 300   | 360.5  | 710.6  | 0      | 1595.2 |
            | f | 600    | 699.9 | 760.5  | 884.6  | 399.9  | 0      |


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

        When I request a travel distance matrix I should get
            |   | a      | b      | f     | 1      |
            | a | 0      | 300    | 900   | 1500   |
            | b | 300    | 0      | 600   | 1200.1 |
            | f | 900    | 600    | 0     | 300    |
            | 1 | 1500   | 1200.1 | 300   | 0      |

        When I request a travel distance matrix I should get
            |   | a      | b      | f     | 1      |
            | a | 0      | 300    | 900   | 1500   |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 300    |
            | f | 900    |
            | 1 | 1500   |

    Scenario: Testbot - Fise input coordinate
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

        When I request a travel distance matrix I should get
            |   | a      | b      | f     | 1      |
            | a | 0      | 300    | 900   | 1500   |
            | b | 300    | 0      | 600   | 1200.1 |
            | f | 900    | 600    | 0     | 300    |
            | 1 | 1500   | 1200.1 | 300   | 0      |

        When I request a travel distance matrix I should get
            |   | a      | b     | f     | 1      |
            | a | 0      | 300   | 900   | 1500   |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 300    |
            | f | 900    |
            | 1 | 1500   |


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

        When I request a travel distance matrix I should get
            |   | a      | b     | f     | 1      |
            | a | 0      | 300   | 900   | 1200   |
            | b | 300    | 0     | 600   | 900    |
            | f | 900    | 600   | 0     | 300  |
            | 1 | 1200   | 900   | 300   | 0      |

        When I request a travel distance matrix I should get
            |   | a      | b     | f     | 1      |
            | a | 0      | 300   | 900   | 1200   |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 300    |
            | f | 900    |
            | 1 | 1200   |

    Scenario: Ensure consistency with route, and make sure offsets work in both directions
        Given a grid size of 100 meters
        Given the node map
            """
            a   b   c   d   e   f   g   h  i  j
                  1                   2       3
            """

        And the ways
            | nodes |
            | abcdef  |
            | fghij  |

        When I route I should get
            | from | to | route              | distance   |
            | 1    | 2  | abcdef,fghij,fghij | 1000.1m    |
            | 1    | 3  | abcdef,fghij,fghij | 1400.1m    |
            | 2    | 3  | fghij,fghij        | 400m       |


        When I request a travel distance matrix I should get
            |   |   1    | 2      | 3      |
            | 1 |   0    | 1000.1 | 1400.1 |
            | 2 | 1000.1 | 0      | 400    |
            | 3 | 1400.1 | 400    | 0      |
