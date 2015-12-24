@matrix @testbot
Feature: Basic Distance Matrix
# note that results are travel time, specified in 1/10th of seconds
# since testbot uses a default speed of 100m/10s, the result matches
# the number of meters as long as the way type is the default 'primary'

    Background:
        Given the profile "testbot"

    Scenario: Testbot - Travel time matrix of minimal network
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I request a travel time matrix I should get
            |   | a   | b   |
            | a | 0   | 100 |
            | b | 100 | 0   |

    Scenario: Testbot - Travel time matrix with different way speeds
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway   |
            | ab    | primary   |
            | bc    | secondary |
            | cd    | tertiary  |
            
        When I request a travel time matrix I should get
            |   | a   | b   | c   | d   |
            | a | 0   | 100 | 300 | 600 |
            | b | 100 | 0   | 200 | 500 |
            | c | 300 | 200 | 0   | 300 |
            | d | 600 | 500 | 300 | 0   |

    Scenario: Testbot - Travel time matrix with fuzzy match
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I request a travel time matrix I should get
            |   | a       | b        |
            | a | 0       | 95 +- 10 |
            | b | 95 ~10% | 0        |
    
    Scenario: Testbot - Travel time matrix of small grid
        Given the node map
            | a | b | c |
            | d | e | f |

        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |

        When I request a travel time matrix I should get
            |   | a   | b   | e   | f   |
            | a | 0   | 100 | 200 | 300 |
            | b | 100 | 0   | 100 | 200 |
            | e | 200 | 100 | 0   | 100 |
            | f | 300 | 200 | 100 | 0   |

    Scenario: Testbot - Travel time matrix of network with unroutable parts
        Given the node map
            | a | b |

        And the ways
            | nodes | oneway |
            | ab    | yes    |

        When I request a travel time matrix I should get
            |   | a  | b   |
            | a | 0  | 100 |
            | b |    | 0   |
    
    Scenario: Testbot - Travel time matrix of network with oneways
        Given the node map
            | x | a | b | y |
            |   | d | e |   |

        And the ways
            | nodes | oneway |
            | abeda | yes    |
            | xa    |        |
            | by    |        |

        When I request a travel time matrix I should get
            |   | x   | y   | d   | e   |
            | x | 0   | 300 | 400 | 300 |
            | y | 500 | 0   | 300 | 200 |
            | d | 200 | 300 | 0   | 300 |
            | e | 300 | 400 | 100 | 0   |

    Scenario: Testbot - Travel time matrix and with only one source
        Given the node map
            | a | b | c |
            | d | e | f |

        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |

        When I request a travel time matrix I should get
            |   | a   | b   | e   | f   |
            | a | 0   | 100 | 200 | 300 |

     Scenario: Testbot - Travel time 3x2 matrix
        Given the node map
            | a | b | c |
            | d | e | f |

        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |

        When I request a travel time matrix I should get
            |   | b   | e   | f   |
            | a | 100 | 200 | 300 |
            | b | 0   | 100 | 200 |

    Scenario: Testbog - All coordinates are from same small component
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the node map
            | a | b |  | f |
            | d | e |  | g |

        And the ways
            | nodes |
            | ab    |
            | be    |
            | ed    |
            | da    |
            | fg    |

        When I request a travel time matrix I should get
            |   | f   | g   |
            | f | 0   | 300 |
            | g | 300 |  0  |

    Scenario: Testbog - Coordinates are from different small component and snap to big CC
        Given a grid size of 300 meters
        Given the extract extra arguments "--small-component-size 4"
        Given the node map
            | a | b |  | f | h |
            | d | e |  | g | i |

        And the ways
            | nodes |
            | ab    |
            | be    |
            | ed    |
            | da    |
            | fg    |
            | hi    |

        When I request a travel time matrix I should get
            |   | f   | g   | h   | i   |
            | f | 0   | 300 | 0   | 300 |
            | g | 300 |  0  | 300 | 0   |
            | h | 0   | 300 | 0   | 300 |
            | i | 300 |  0  | 300 | 0   |

