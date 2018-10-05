@matrix @testbot
Feature: Basic Distance Matrix
# note that results of travel distance are in metres

    Background:
        Given the profile "testbot"
        And the partition extra arguments "--small-component-size 1 --max-cell-sizes 2,4,8,16"
  # Scenario: Testbot - Travel distance matrix of small grid
  #       Given the node map
  #           """
  #           a b c
  #           d e f
  #           """

  #       And the ways
  #           | nodes |
  #           | abc   |
  #           | def   |
  #           | ad    |
  #           | be    |
  #           | cf    |

  #       When I request a travel distance matrix I should get
  #           |   | a      | b      | e      | f      |
  #           | a | 0      | 100+-1 | 200+-1 | 300+-1 |
  #           | b | 100+-1 | 0      | 100+-1 | 200+-1 |
  #           | e | 200+-1 | 100+-1 | 0      | 100+-1 |
  #           | f | 300+-1 | 200+-1 | 100+-1 | 0      |

    Scenario: Testbot - Travel distance matrix of minimal network
        Given the node map
            """
            a z   
              b
            """

        And the ways
            | nodes |
            | az    |
            | zb    |

        When I request a travel distance matrix I should get
            |   | a      | b      |
            | a | 0      | 400+-1 |
            | b | 400+-1 | 0      |

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
            |   | a      | b      | c      | d      |
            | a | 0      | 100+-1 |        |        |
            | b | 100+-1 | 0      |        |        |
            | c |        |        | 0      | 100+-1 |
            | d |        |        | 100+-1 | 0      |

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
            |   | a | b      | c      | d      |
            | a | 0 | 300+-2 | 100+-2 | 200+-2 |

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
            |   | a | b     | e |
            | a | 0 | 50+-1 |   |

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
            | a | 0 | 100+-1 |   |   |

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
            |   | a      | b      | c      | d      |
            | a | 0      | 100+-1 | 200+-1 | 300+-1 |
            | b | 100+-1 | 0      | 100+-1 | 200+-1 |
            | c | 200+-1 | 100+-1 | 0      | 100+-1 |
            | d | 300+-1 | 200+-1 | 100+-1 | 0     |

        When I request a travel distance matrix I should get
            |   | a | b      | c      | d      |
            | a | 0 | 100+-1 | 200+-1 | 300+-1 |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 100+-1 |
            | c | 200+-1 |
            | d | 300+-1 |

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
            |   | a      | b      | e      | f      |
            | a | 0      | 100+-1 | 200+-1 | 300+-1 |
            | b | 100+-1 | 0      | 100+-1 | 200+-1 |
            | e | 200+-1 | 100+-1 | 0      | 100+-1 |
            | f | 300+-1 | 200+-1 | 100+-1 | 0      |

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
            | a | 0 | 100+-1 |
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
            |   | x      | y      | d      | e      |
            | x | 0      | 300+-2 | 400+-2 | 300+-2 |
            | y | 500+-2 | 0      | 300+-2 | 200+-2 |
            | d | 200+-2 | 300+-2 | 0      | 300+-2 |
            | e | 300+-2 | 400+-2 | 100+-2 | 0      |

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
            | e    | a  | 200m +- 1 |
            | e    | b  | 100m +- 1 |
            | f    | a  | 300m +- 1 |
            | f    | b  | 200m +- 1 |

        When I request a travel distance matrix I should get
            |   | a | b      | e      | f      |
            | a | 0 | 100+-1 | 200+-1 | 300+-1 |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 100+-1 |
            | e | 200+-1 |
            | f | 300+-1 |

        When I request a travel distance matrix I should get
            |   | a      | b      | e      | f      |
            | a | 0      | 100+-1 | 200+-1 | 300+-1 |
            | b | 100+-1 | 0      | 100+-1 | 200+-1 |

        When I request a travel distance matrix I should get
            |   | a      | b      |
            | a | 0      | 100+-1 |
            | b | 100+-1 | 0      |
            | e | 200+-1 | 100+-1 |
            | f | 300+-1 | 200+-1 |

        When I request a travel distance matrix I should get
            |   | a      | b      | e      | f      |
            | a | 0      | 100+-1 | 200+-1 | 300+-1 |
            | b | 100+-1 | 0      | 100+-1 | 200+-1 |
            | e | 200+-1 | 100+-1 | 0      | 100+-1 |

        When I request a travel distance matrix I should get
            |   | a      | b      | e      |
            | a | 0      | 100+-1 | 200+-1 |
            | b | 100+-1 | 0      | 100+-1 |
            | e | 200+-1 | 100+-1 | 0      |
            | f | 300+-1 | 200+-1 | 100+-1 |

        When I request a travel distance matrix I should get
            |   | a      | b      | e      | f      |
            | a | 0      | 100+-1 | 200+-1 | 300+-1 |
            | b | 100+-1 | 0      | 100+-1 | 200+-1 |
            | e | 200+-1 | 100+-1 | 0      | 100+-1 |
            | f | 300+-1 | 200+-1 | 100+-1 | 0      |

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
            |   | b      | e      | f      |
            | a | 100+-1 | 200+-1 | 300+-1 |
            | b | 0      | 100+-1 | 200+-1 |

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
            |   | f      | g      |
            | f | 0      | 300+-2 |
            | g | 300+-2 | 0      |

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
            |   | f      | g      | h      | i      |
            | f | 0      | 300+-2 | 0      | 300+-2 |
            | g | 300+-2 | 0      | 300+-2 | 0      |
            | h | 0      | 300+-2 | 0      | 300+-2 |
            | i | 300+-2 | 0      | 300+-2 | 0      |

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
            |   | 1      | 2      | 3      | 4      |
            | 1 | 0      | 100+-1 | 400+-1 | 500+-1 |
            | 2 | 700+-1 | 0      | 300+-1 | 400+-1 |
            | 3 | 400+-1 | 500+-1 | 0      | 100+-1 |
            | 4 | 300+-1 | 400+-1 | 700+-1 | 0      |


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
            |   | a      | b      | c      | d      | e      |
            | a | 0      | 100+-2 | 200+-2 | 300+-2 | 400+-2 |
            | b | 100+-2 | 0      | 100+-2 | 200+-2 | 300+-2 |
            | c | 200+-2 | 100+-2 | 0      | 100+-2 | 200+-2 |
            | d | 300+-2 | 200+-2 | 100+-2 | 0      | 300+-2 |
            | e | 400+-2 | 300+-2 | 200+-2 | 300+-2 | 0      |

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
            |   | 1       | 2       | 3       | 4       | 5       | 6       | 7       | 8       |
            | 1 | 0       | 1100+-5 | 300+-5  | 200+-5  | 600+-5  | 500+-5  | 900+-5  | 800+-5  |
            | 2 | 100+-5  | 0       | 400+-5  | 300+-5  | 700+-5  | 600+-5  | 1000+-5 | 900+-5  |
            | 3 | 900+-5  | 800+-5  | 0       | 1100+-5 | 300+-5  | 200+-5  | 600+-5  | 500+-5  |
            | 4 | 1000+-5 | 900+-5  | 100+-5  | 0       | 400+-5  | 300+-5  | 700+-5  | 600+-5  |
            | 5 | 600+-5  | 500+-5  | 900+-5  | 800+-5  | 0       | 1100+-5 | 300+-5  | 200+-5  |
            | 6 | 700+-5  | 600+-5  | 1000+-5 | 900+-5  | 100+-5  | 0       | 400+-5  | 300+-5  |
            | 7 | 300+-5  | 200+-5  | 600+-5  | 500+-5  | 900+-5  | 800+-5  | 0       | 1100+-5 |
            | 8 | 400+-5  | 300+-5  | 700+-5  | 600+-5  | 1000+-5 | 900+-5  | 100+-5  | 0       |

        When I request a travel distance matrix I should get
            |   | 1       |
            | 1 | 0       |
            | 2 | 100+-5  |
            | 3 | 900+-5  |
            | 4 | 1000+-5 |
            | 5 | 600+-5  |
            | 6 | 700+-5  |
            | 7 | 300+-5  |
            | 8 | 400+-5  |

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
            | from | to | route    | distance  |
            | a    | b  | ab,ab    | 450m      |
            | a    | c  | ac,ac    | 200m      |
            | a    | d  | ac,dc,dc | 500m +- 1 |

        When I request a travel distance matrix I should get
            |   | a | b      | c      | d      |
            | a | 0 | 450+-2 | 200+-2 | 500+-2 |

        When I request a travel distance matrix I should get
            |   | a      |
            | a | 0      |
            | b | 450+-2 |
            | c | 200+-2 |
            | d | 500+-2 |

        When I request a travel distance matrix I should get
            |   | a      | c      |
            | a | 0      | 200+-2 |
            | c | 200+-2 | 0      |


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
            |   | a | b       | c       | d       |
            | a | 0 | 1000+-3 | 2000+-3 | 3000+-3 |


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
            |   | a       | b       | c       | d       | e       | f       |
            | a | 0       | 100+-1  | 300+-1  | 650+-1  | 1930+-1 | 1533+-1 |
            | b | 760+-1  | 0       | 200+-1  | 550+-1  | 1830+-1 | 1433+-1 |
            | c | 560+-2  | 660+-2  | 0       | 350+-1  | 1630+-1 | 1233+-1 |
            | d | 1480+-2 | 1580+-1 | 1780+-1 | 0       | 1280+-1 | 883+-1  |
            | e | 200+-2  | 300+-2  | 500+-1  | 710+-1  | 0       | 1593+-1 |
            | f | 597+-1  | 696+-1  | 896+-1  | 1108+-1 | 400+-3  | 0       |
