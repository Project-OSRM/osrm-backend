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
            | a |    0    |  100.1  |  199.5  |  299.5 |
            | b |  100.1  |   0     |  99.4   |  199.5 |
            | e |  199.5  |  99.4   |   0     |  100.1 |
            | f |  299.5  |  199.5  |  100.1  |   0    |

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
            | a |   0    |  100.1  |  199.5  |  298.9  |  398.3  |
            | z |  100.1 |  0      |  99.4   |  198.8  |  298.2  |
            | b |  199.5 |  99.4   |  0      |  99.4   |  198.8  |
            | c |  298.9 |  198.8  |  99.4   |  0      |  99.4   |
            | d |  398.3 |  298.2  |  198.8  |  99.4   |  0      |

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
            | a | 0     | 100.1 |       |       |
            | b | 100.1 | 0     |       |       |
            | c |       |       | 0     | 100.1 |
            | d |       |       | 100.1 | 0     |

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
            | a | 0 |  298.9 | 99.4 | 199.5 |

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
            | a | 0 | 50.1 |   |

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
            | a | 0 | 100.1  |   |   |

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
            | a | 0     | 100.1 | 200.1 | 300.2 |
            | b | 100.1 | 0     | 100.1 | 200.1 |
            | c | 200.1 | 100.1 | 0     | 100.1 |
            | d | 300.2 | 200.1 | 100.1 | 0     |

        When I request a travel distance matrix I should get
            |   | a | b     | c     | d     |
            | a | 0 | 100.1 | 200.1 | 300.2 |

        When I request a travel distance matrix I should get
            |   | a     |
            | a | 0     |
            | b | 100.1 |
            | c | 200.1 |
            | d | 300.2 |

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
            | a | 0     | 100.1 | 199.5 | 299.5 |
            | b | 100.1 | 0     | 99.4  | 199.5 |
            | e | 199.5 | 99.4  | 0     | 100.1 |
            | f | 299.5 | 199.5 | 100.1 | 0     |

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
            | a | 0 | 100.1  |
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
            | x | 0     | 300.2 | 399.6 | 299.5 |
            | y | 499   | 0     | 299.5 | 199.5 |
            | d | 199.5 | 299.5 | 0     | 298.9 |
            | e | 299.5 | 399.6 | 100.1 | 0     |

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
            | e    | a  | 200m      |
            | e    | b  | 100m      |
            | f    | a  | 299.9m    |
            | f    | b  | 200m      |

        When I request a travel distance matrix I should get
            |   | a | b      | e      | f      |
            | a | 0 |  100.1 |  199.5 |  299.5 |

        When I request a travel distance matrix I should get
            |   | a     |
            | a | 0     |
            | b | 100.1 |
            | e | 199.5 |
            | f | 299.5 |

        When I request a travel distance matrix I should get
            |   | a     | b     | e     | f     |
            | a | 0     | 100.1 | 199.5 | 299.5 |
            | b | 100.1 | 0     | 99.4  | 199.5 |

        When I request a travel distance matrix I should get
            |   | a     | b     |
            | a | 0     | 100.1 |
            | b | 100.1 | 0     |
            | e | 199.5 | 99.4  |
            | f | 299.5 | 199.5 |

        When I request a travel distance matrix I should get
            |   | a     | b     | e     | f     |
            | a | 0     | 100.1 | 199.5 | 299.5 |
            | b | 100.1 | 0     | 99.4  | 199.5 |
            | e | 199.5 | 99.4  | 0     | 100.1 |

        When I request a travel distance matrix I should get
             |   | a     | b     | e     |
             | a | 0     | 100.1 | 199.5 |
             | b | 100.1 | 0     | 99.4  |
             | e | 199.5 | 99.4  | 0     |
             | f | 299.5 | 199.5 | 100.1 |

        When I request a travel distance matrix I should get
             |   | a     | b     | e     | f     |
             | a | 0     | 100.1 | 199.5 | 299.5 |
             | b | 100.1 | 0     | 99.4  | 199.5 |
             | e | 199.5 | 99.4  | 0     | 100.1 |
             | f | 299.5 | 199.5 | 100.1 | 0     |

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
            | a | 100.1 | 199.5 | 299.5 |
            | b | 0     | 99.4  | 199.5 |

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
            | f | 0     | 298.2 |
            | g | 298.2 | 0     |

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
            | f | 0     | 298.2 | 0     | 298.2 |
            | g | 298.2 | 0     | 298.2 | 0     |
            | h | 0     | 298.2 | 0     | 298.2 |
            | i | 298.2 | 0     | 298.2 | 0     |

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
            | 1 | 0     | 100.1 | 399.6 | 499.7 |
            | 2 | 699.1 | 0     | 299.5 | 399.6 |
            | 3 | 399.6 | 499.7 | 0     | 100.1 |
            | 4 | 299.5 | 399.6 | 699.1 | 0     |


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
            | a | 0     | 100.1 | 200.1 | 300.2 | 398.9 |
            | b | 100.1 | 0     | 100.1 | 200.1 | 298.9 |
            | c | 200.1 | 100.1 | 0     | 100.1 | 198.8 |
            | d | 300.2 | 200.1 | 100.1 | 0     | 298.9 |
            | e | 398.9 | 298.9 | 198.8 | 298.9 | 0     |

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
             | 1 | 0     | 1096.7 | 298.9 | 199.5  | 598.4 | 498.3  | 897.3 | 797.9  |
             | 2 | 100.1 | 0      | 398.9 | 299.5  | 698.5 | 598.4  | 997.3 | 897.9  |
             | 3 | 897.9 | 797.9  | 0     | 1097.4 | 299.5 | 199.5  | 598.4 | 499    |
             | 4 | 997.3 | 897.3  | 99.4  | 0      | 398.9 | 298.9  | 697.8 | 598.4  |
             | 5 | 598.4 | 498.3  | 897.3 | 797.9  | 0     | 1096.7 | 298.9 | 199.5  |
             | 6 | 698.5 | 598.4  | 997.3 | 897.9  | 100.1 | 0      | 398.9 | 299.5  |
             | 7 | 299.5 | 199.5  | 598.4 | 499    | 897.9 | 797.9  | 0     | 1097.4 |
             | 8 | 398.9 | 298.9  | 697.8 | 598.4  | 997.3 | 897.3  | 99.4  | 0      |

        When I request a travel distance matrix I should get
             |   | 1     |
             | 1 | 0     |
             | 2 | 100.1 |
             | 3 | 897.9 |
             | 4 | 997.3 |
             | 5 | 598.4 |
             | 6 | 698.5 |
             | 7 | 299.5 |
             | 8 | 398.9 |

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
            | a    | d  | ac,dc,dc | 499.9m   |

        When I request a travel distance matrix I should get
            |   | a | b     | c     | d   |
            | a | 0 | 450.3 | 198.8 | 499 |

        When I request a travel distance matrix I should get
            |   | a     |
            | a | 0     |
            | b | 450.3 |
            | c | 198.8 |
            | d | 499   |

        When I request a travel distance matrix I should get
            |   | a     | c     |
            | a | 0     | 198.8 |
            | c | 198.8 | 0     |


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
            | a | 0 | 1000.7 | 2001.4 | 3002.1 |


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
            | b | 240.4 | 0      |

        When I request a travel distance matrix I should get
            |   |   a   |
            | a |   0   |
            | b | 240.4 |

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
            | a | 0      | 100.1 | 300.2  | 650.5  | 1930.6 | 1533   |
            | b | 759    | 0     | 200.1  | 550.4  | 1830.5 | 1432.9 |
            | c | 558.8  | 658.9 | 0      | 350.3  | 1630.4 | 1232.8 |
            | d | 1478.9 | 1579  | 1779.1 | 0      | 1280.1 | 882.5  |
            | e | 198.8  | 298.9 | 499    | 710.3  | 0      | 1592.8 |
            | f | 596.4  | 696.5 | 896.6  | 1107.9 | 397.6  | 0      |

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
            | a | 0      | 300.2  | 900.7 | 1501.1 |
            | b | 300.2  | 0      | 600.5 | 1200.9 |
            | f | 900.7  | 600.5  | 0     | 300.2  |
            | 1 | 1501.1 | 1200.9 | 300.2 | 0      |

        When I request a travel distance matrix I should get
            |   | a      | b      | f     | 1      |
            | a | 0      | 300.2  | 900.7 | 1501.1 |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 300.2  |
            | f | 900.7  |
            | 1 | 1501.1 |

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
            | a | 0      | 300.2  | 900.7 | 1501.1 |
            | b | 300.2  | 0      | 600.5 | 1200.9 |
            | f | 900.7  | 600.5  | 0     | 300.2  |
            | 1 | 1501.1 | 1200.9 | 300.2 | 0      |

        When I request a travel distance matrix I should get
            |   | a      | b     | f     | 1      |
            | a | 0      | 300.2 | 900.7 | 1501.1 |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 300.2  |
            | f | 900.7  |
            | 1 | 1501.1 |


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
            | a | 0      | 300.2 | 900.7 | 1200.9 |
            | b | 300.2  | 0     | 600.5 | 900.7  |
            | f | 900.7  | 600.5 | 0     | 300.2  |
            | 1 | 1200.9 | 900.7 | 300.2 | 0      |

        When I request a travel distance matrix I should get
            |   | a      | b     | f     | 1      |
            | a | 0      | 300.2 | 900.7 | 1200.9 |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 300.2  |
            | f | 900.7  |
            | 1 | 1200.9 |

    Scenario: Ensure consistency with route, and make sure offsets work in both directions
        Given a grid size of 100 meters
        Given the node map
            """
            a   b   c   d   e   f   g   h  i  j
                  1                   2
            """

        And the ways
            | nodes |
            | abcdef  |
            | fghij  |

        When I route I should get
            | from | to | route              | distance |
            | 1    | 2  | abcdef,fghij,fghij | 999.9m  |

        # TODO: this is "correct", but inconsistent with viaroute
        When I request a travel distance matrix I should get
            |   |   1    | 2      |
            | 1 |   0    | 1000.7 |
            | 2 | 1000.7 | 0      |