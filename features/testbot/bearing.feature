@routing @bearing @testbot
Feature: Compass bearing

    Background:
        Given the profile "testbot"
        Given a grid size of 200 meters

    Scenario: Bearing when going northwest
        Given the node map
            """
            b
              a
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route    | bearing      |
            | a    | b  | ab,ab    | 0->315,315->0|

    Scenario: Bearing when going west
        Given the node map
            """
            b a
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route    | bearing      |
            | a    | b  | ab,ab    | 0->270,270->0|

    Scenario: Bearing af 45 degree intervals
        Given the node map
            """
            b a h
            c x g
            d e f
            """

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
            | from | to | route    | bearing |
            | x    | a  | xa,xa    | 0->0,0->0|
            | x    | b  | xb,xb    | 0->315,315->0|
            | x    | c  | xc,xc    | 0->270,270->0|
            | x    | d  | xd,xd    | 0->225,225->0|
            | x    | e  | xe,xe    | 0->180,180->0|
            | x    | f  | xf,xf    | 0->135,135->0|
            | x    | g  | xg,xg    | 0->90,90->0|
            | x    | h  | xh,xh    | 0->45,45->0|

    Scenario: Bearing in a roundabout
        Given the node map
            """
            k d c j
            e     b
            f     a
            l g h i
            """

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
            | dk    | no     |
            | ke    | no     |
            | fl    | no     |
            | lg    | no     |
            | hi    | no     |
            | ia    | no     |
            | bj    | no     |
            | cj    | no     |

        When I route I should get
            | from | to | route                   | bearing                                                     |
            | c    | b  | cd,de,ef,fg,gh,ha,ab,ab | 0->270,270->225,225->180,180->135,135->90,90->45,45->0,0->0 |
            | g    | f  | gh,ha,ab,bc,cd,de,ef,ef | 0->90,90->45,45->0,0->315,315->270,270->225,225->180,180->0 |

    Scenario: Bearing should stay constant when zig-zagging
        Given the node map
            """
            i j k
            b d f h
            a c e g
              m n o
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | bi    |
            | cm    |
            | dj    |
            | en    |
            | fk    |
            | go    |

        When I route I should get
            | from | to | route                   | bearing               |
            | a    | h  | ab,bc,cd,de,ef,fg,gh,gh | 0->0,0->135,135->0,0->135,135->0,0->135,135->0,0->0 |

    Scenario: Bearings on an east-west way.
        Given the node map
            """
            a b c d e f
            """

        And the ways
            | nodes  |
            | abcdef |

        When I route I should get
            | from | to | route         | bearing       |
            | a    | b  | abcdef,abcdef | 0->90,90->0   |
            | a    | c  | abcdef,abcdef | 0->90,90->0   |
            | a    | d  | abcdef,abcdef | 0->90,90->0   |
            | a    | e  | abcdef,abcdef | 0->90,90->0   |
            | a    | f  | abcdef,abcdef | 0->90,90->0   |
            | b    | a  | abcdef,abcdef | 0->270,270->0 |
            | b    | c  | abcdef,abcdef | 0->90,90->0   |
            | b    | d  | abcdef,abcdef | 0->90,90->0   |
            | b    | e  | abcdef,abcdef | 0->90,90->0   |
            | b    | f  | abcdef,abcdef | 0->90,90->0   |
            | c    | a  | abcdef,abcdef | 0->270,270->0 |
            | c    | b  | abcdef,abcdef | 0->270,270->0 |
            | c    | d  | abcdef,abcdef | 0->90,90->0   |
            | c    | e  | abcdef,abcdef | 0->90,90->0   |
            | c    | f  | abcdef,abcdef | 0->90,90->0   |
            | d    | a  | abcdef,abcdef | 0->270,270->0 |
            | d    | b  | abcdef,abcdef | 0->270,270->0 |
            | d    | c  | abcdef,abcdef | 0->270,270->0 |
            | d    | e  | abcdef,abcdef | 0->90,90->0   |
            | d    | f  | abcdef,abcdef | 0->90,90->0   |
            | e    | a  | abcdef,abcdef | 0->270,270->0 |
            | e    | b  | abcdef,abcdef | 0->270,270->0 |
            | e    | c  | abcdef,abcdef | 0->270,270->0 |
            | e    | d  | abcdef,abcdef | 0->270,270->0 |
            | e    | f  | abcdef,abcdef | 0->90,90->0   |
            | f    | a  | abcdef,abcdef | 0->270,270->0 |
            | f    | b  | abcdef,abcdef | 0->270,270->0 |
            | f    | c  | abcdef,abcdef | 0->270,270->0 |
            | f    | d  | abcdef,abcdef | 0->270,270->0 |
            | f    | e  | abcdef,abcdef | 0->270,270->0 |

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
            | from | to | route | bearing         |
            | a    | b  | ab,ab | 0->0,0->0       |
            | b    | c  | bc,bc | 0->90,90->0     |
            | c    | d  | cd,cd | 0->180,180->0   |
            | d    | a  | da,da | 0->270,270->0   |
            | b    | a  | ab,ab | 0->180,180->0   |
            | c    | b  | bc,bc | 0->270,270->0   |
            | d    | c  | cd,cd | 0->0,0->0       |
            | a    | d  | da,da | 0->90,90->0     |
            | a    | c  | ac,ac | 0->45,45->0     |
            | c    | a  | ac,ac | 0->225,225->0   |
            | b    | d  | bd,bd | 0->135,135->0   |
            | d    | b  | bd,bd | 0->315,315->0   |

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
            | from | to | route  | bearing         |
            | a    | b  | ab,ab  | 0->180,180->0   |
            | b    | c  | bc,bc  | 0->90,90->0     |
            | c    | d  | cd,cd  | 0->0,0->0       |
            | d    | a  | da,da  | 0->270,270->0   |
            | b    | a  | ab,ab  | 0->0,0->0       |
            | c    | b  | bc,bc  | 0->270,270->0   |
            | d    | c  | cd,cd  | 0->180,180->0   |
            | a    | d  | da,da  | 0->90,90->0     |
            | a    | c  | ac,ac  | 0->135,135->0   |
            | c    | a  | ac,ac  | 0->315,315->0   |
            | b    | d  | bd,bd  | 0->45,45->0     |
            | d    | b  | bd,bd  | 0->225,225->0   |
