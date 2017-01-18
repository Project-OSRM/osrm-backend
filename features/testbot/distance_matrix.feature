@matrix @testbot
Feature: Basic Distance Matrix
# note that results are travel time, specified in 1/10th of seconds
# since testbot uses a default speed of 100m/10s, the result matches
# the number of meters as long as the way type is the default 'primary'

    Background:
        Given the profile "testbot.lua"

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

    Scenario: Testbot - Travel time matrix with fuzzy match
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I request a travel time matrix I should get
            |   | a       | b        |
            | a | 0       | 9 +- 2   |
            | b | 9  ~15% | 0        |

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

    Scenario: Testbot - Travel time matrix and with only one source
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
            |   | 1      | 2      | 3      | 4  |
            | 1 | 0      | 10 +-1 | 40 +-1 | 50 +-1 |
            | 2 | 70 +-1 | 0      | 30 +-1 | 40 +-1 |
            | 3 | 40 +-1 | 50 +-1 | 0      | 10 +-1 |
            | 4 | 30 +-1 | 40 +-1 | 70 +-1 | 0  |
