@routing @bearing @testbot
Feature: Compass bearing

    Background:
        Given the profile "testbot"

    Scenario: Bearing when going northwest
        Given the node map
            | b |   |
            |   | a |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | bearing |
            | a    | b  | ab    | 315     |

    Scenario: Bearing when going west
        Given the node map
            | b | a |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | bearing |
            | a    | b  | ab    | 270     |

    Scenario: Bearing af 45 degree intervals
        Given the node map
            | b | a | h |
            | c | x | g |
            | d | e | f |

        And the ways
            | nodes |
            | xa    |
            | xb    |
            | xc    |
            | xd    |
            | xe    |
            | xf    |
            | xg    |
            | xh    |

        When I route I should get
            | from | to | route | bearing |
            | x    | a  | xa    | 0       |
            | x    | b  | xb    | 315     |
            | x    | c  | xc    | 270     |
            | x    | d  | xd    | 225     |
            | x    | e  | xe    | 180     |
            | x    | f  | xf    | 135     |
            | x    | g  | xg    | 90      |
            | x    | h  | xh    | 45      |

    Scenario: Bearing in a roundabout
        Given the node map
            |   | d | c |   |
            | e |   |   | b |
            | f |   |   | a |
            |   | g | h |   |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fg    | yes    |
            | gh    | yes    |
            | ha    | yes    |

        When I route I should get
            | from | to | route                | bearing                 |
            | c    | b  | cd,de,ef,fg,gh,ha,ab | 270,225,180,135,90,45,0 |
            | g    | f  | gh,ha,ab,bc,cd,de,ef | 90,45,0,315,270,225,180 |

    Scenario: Bearing should stay constant when zig-zagging
        Given the node map
            | b | d | f | h |
            | a | c | e | g |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | de    |
            | ef    |
            | fg    |
            | gh    |

        When I route I should get
            | from | to | route                | bearing             |
            | a    | h  | ab,bc,cd,de,ef,fg,gh | 0,135,0,135,0,135,0 |

    Scenario: Bearings on an east-west way.
        Given the node map
            | a | b | c | d | e | f |

        And the ways
            | nodes  |
            | abcdef |

        When I route I should get
            | from | to | route  | bearing |
            | a    | b  | abcdef | 90      |
            | a    | c  | abcdef | 90      |
            | a    | d  | abcdef | 90      |
            | a    | e  | abcdef | 90      |
            | a    | f  | abcdef | 90      |
            | b    | a  | abcdef | 270     |
            | b    | c  | abcdef | 90      |
            | b    | d  | abcdef | 90      |
            | b    | e  | abcdef | 90      |
            | b    | f  | abcdef | 90      |
            | c    | a  | abcdef | 270     |
            | c    | b  | abcdef | 270     |
            | c    | d  | abcdef | 90      |
            | c    | e  | abcdef | 90      |
            | c    | f  | abcdef | 90      |
            | d    | a  | abcdef | 270     |
            | d    | b  | abcdef | 270     |
            | d    | c  | abcdef | 270     |
            | d    | e  | abcdef | 90      |
            | d    | f  | abcdef | 90      |
            | e    | a  | abcdef | 270     |
            | e    | b  | abcdef | 270     |
            | e    | c  | abcdef | 270     |
            | e    | d  | abcdef | 270     |
            | e    | f  | abcdef | 90      |
            | f    | a  | abcdef | 270     |
            | f    | b  | abcdef | 270     |
            | f    | c  | abcdef | 270     |
            | f    | d  | abcdef | 270     |
            | f    | e  | abcdef | 270     |

    Scenario: Bearings at high latitudes
    # The coordinas below was calculated using http://www.movable-type.co.uk/scripts/latlong.html,
    # to form square with sides of 1 km.

        Given the node locations
            | node | lat       | lon      |
            | a    | 80        | 0        |
            | b    | 80.006389 | 0        |
            | c    | 80.006389 | 0.036667 |
            | d    | 80        | 0.036667 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | da    |
            | ac    |
            | bd    |

        When I route I should get
            | from | to | route | bearing |
            | a    | b  | ab    | 0       |
            | b    | c  | bc    | 90      |
            | c    | d  | cd    | 180     |
            | d    | a  | da    | 270     |
            | b    | a  | ab    | 180     |
            | c    | b  | bc    | 270     |
            | d    | c  | cd    | 0       |
            | a    | d  | da    | 90      |
            | a    | c  | ac    | 45      |
            | c    | a  | ac    | 225     |
            | b    | d  | bd    | 135     |
            | d    | b  | bd    | 315     |

    Scenario: Bearings at high negative latitudes
    # The coordinas below was calculated using http://www.movable-type.co.uk/scripts/latlong.html,
    # to form square with sides of 1 km.

        Given the node locations
            | node | lat        | lon      |
            | a    | -80        | 0        |
            | b    | -80.006389 | 0        |
            | c    | -80.006389 | 0.036667 |
            | d    | -80        | 0.036667 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | da    |
            | ac    |
            | bd    |

        When I route I should get
            | from | to | route | bearing |
            | a    | b  | ab    | 180     |
            | b    | c  | bc    | 90      |
            | c    | d  | cd    | 0       |
            | d    | a  | da    | 270     |
            | b    | a  | ab    | 0       |
            | c    | b  | bc    | 270     |
            | d    | c  | cd    | 180     |
            | a    | d  | da    | 90      |
            | a    | c  | ac    | 135     |
            | c    | a  | ac    | 315     |
            | b    | d  | bd    | 45      |
            | d    | b  | bd    | 225     |
