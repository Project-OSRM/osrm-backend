@routing @distance @testbot
Feature: Distance calculation

    Background:
        Given the profile "testbot"

    Scenario: 100m distance
        Given a grid size of 100 meters
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | distance |
            | a    | b  | ab,ab | 100m     |

    Scenario: Distance should equal sum of segments, leftwinded
        Given the node map
            """
            e
            d c
            a b
            """

        And the ways
            | nodes |
            | abcde |

        When I route I should get
            | from | to | route       | distance |
            | a    | d  | abcde,abcde | 300m +-1 |

    Scenario: Distance should equal sum of segments, rightwinded
        Given the node map
            """
              e
            c d
            b a
            """

        And the ways
            | nodes |
            | abcde |

        When I route I should get
            | from | to | route       | distance |
            | a    | d  | abcde,abcde | 300m +-1 |

    Scenario: 10m distances
        Given a grid size of 10 meters
        Given the node map
            """
            a b
              c
            """

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | from | to | route   | distance |
            | a    | b  | abc,abc | 10m      |
            | b    | a  | abc,abc | 10m      |
            | b    | c  | abc,abc | 10m      |
            | c    | b  | abc,abc | 10m      |
            | a    | c  | abc,abc | 20m      |
            | c    | a  | abc,abc | 20m      |

    Scenario: 100m distances
        Given a grid size of 100 meters
        Given the node map
            """
            a b
              c
            """

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | from | to | route   | distance |
            | a    | b  | abc,abc | 100m     |
            | b    | a  | abc,abc | 100m     |
            | b    | c  | abc,abc | 100m     |
            | c    | b  | abc,abc | 100m     |
            | a    | c  | abc,abc | 200m     |
            | c    | a  | abc,abc | 200m     |

    Scenario: 1km distance
        Given a grid size of 1000 meters
        Given the node map
            """
            a b
              c
            """

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | from | to | route   | distance  |
            | a    | b  | abc,abc | 1000m +-1 |
            | b    | a  | abc,abc | 1000m +-1 |
            | b    | c  | abc,abc | 1000m +-1 |
            | c    | b  | abc,abc | 1000m +-1 |
            | a    | c  | abc,abc | 2000m +-1 |
            | c    | a  | abc,abc | 2000m +-1 |

    Scenario: Distance of a winding south-north path
        Given a grid size of 10 meters
        Given the node map
            """
            a b
            d c
            e f
            h g
            """

        And the ways
            | nodes    |
            | abcdefgh |

        When I route I should get
            | from | to | route             | distance |
            | a    | b  | abcdefgh,abcdefgh | 10m      |
            | a    | c  | abcdefgh,abcdefgh | 20m      |
            | a    | d  | abcdefgh,abcdefgh | 30m      |
            | a    | e  | abcdefgh,abcdefgh | 40m      |
            | a    | f  | abcdefgh,abcdefgh | 50m      |
            | a    | g  | abcdefgh,abcdefgh | 60m +-1  |
            | a    | h  | abcdefgh,abcdefgh | 70m +-1  |

    Scenario: Distance of a winding east-west path
        Given a grid size of 10 meters
        Given the node map
            """
            a d e h
            b c f g
            """

        And the ways
            | nodes    |
            | abcdefgh |

        When I route I should get
            | from | to | route             | distance |
            | a    | b  | abcdefgh,abcdefgh | 10m      |
            | a    | c  | abcdefgh,abcdefgh | 20m      |
            | a    | d  | abcdefgh,abcdefgh | 30m      |
            | a    | e  | abcdefgh,abcdefgh | 40m      |
            | a    | f  | abcdefgh,abcdefgh | 50m      |
            | a    | g  | abcdefgh,abcdefgh | 60m +-1  |
            | a    | h  | abcdefgh,abcdefgh | 70m +-1  |

    Scenario: Geometric distances
        Given a grid size of 100 meters
        Given the node map
            """
            v w y a b c d
            u           e
            t           f
            s     x     g
            r           h
            q           i
            p o n m l k j
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
            | xi    |
            | xj    |
            | xk    |
            | xl    |
            | xm    |
            | xn    |
            | xo    |
            | xp    |
            | xq    |
            | xr    |
            | xs    |
            | xt    |
            | xu    |
            | xv    |
            | xw    |
            | xy    |

        When I route I should get
            | from | to | route | distance |
            | x    | a  | xa,xa | 300m +-1 |
            | x    | b  | xb,xb | 316m +-1 |
            | x    | c  | xc,xc | 360m +-1 |
            | x    | d  | xd,xd | 424m +-1 |
            | x    | e  | xe,xe | 360m +-1 |
            | x    | f  | xf,xf | 316m +-1 |
            | x    | g  | xg,xg | 300m +-1 |
            | x    | h  | xh,xh | 316m +-1 |
            | x    | i  | xi,xi | 360m +-1 |
            | x    | j  | xj,xj | 424m +-1 |
            | x    | k  | xk,xk | 360m +-1 |
            | x    | l  | xl,xl | 316m +-1 |
            | x    | m  | xm,xm | 300m +-1 |
            | x    | n  | xn,xn | 316m +-1 |
            | x    | o  | xo,xo | 360m +-1 |
            | x    | p  | xp,xp | 424m +-1 |
            | x    | q  | xq,xq | 360m +-1 |
            | x    | r  | xr,xr | 316m +-1 |
            | x    | s  | xs,xs | 300m +-1 |
            | x    | t  | xt,xt | 316m +-1 |
            | x    | u  | xu,xu | 360m +-1 |
            | x    | v  | xv,xv | 424m +-1 |
            | x    | w  | xw,xw | 360m +-1 |
            | x    | y  | xy,xy | 316m +-1 |

    @maze
    Scenario: Distance of a maze of short segments
        Given a grid size of 7 meters
        Given the node map
            """
            a b s t
            d c r q
            e f o p
            h g n m
            i j k l
            """

        And the ways
            | nodes                |
            | abcdefghijklmnopqrst |

        When I route I should get
            | from | to | route                                     | distance |
            | a    | t  | abcdefghijklmnopqrst,abcdefghijklmnopqrst | 133m +-1 |
