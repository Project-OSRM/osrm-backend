@routing @time @testbot
Feature: Estimation of travel time
# Testbot speeds:
# Primary road:    36km/h = 36000m/3600s = 100m/10s
# Secondary road:    18km/h = 18000m/3600s = 100m/20s
# Tertiary road:    12km/h = 12000m/3600s = 100m/30s

    Background: Use specific speeds
        Given the profile "testbot"

    Scenario: Basic travel time, 10m scale
        Given a grid size of 10 meters
        Given the node map
            """
            h a b
            g x c
            f e d
            """

        And the ways
            | nodes | highway |
            | xa    | primary |
            | xb    | primary |
            | xc    | primary |
            | xd    | primary |
            | xe    | primary |
            | xf    | primary |
            | xg    | primary |
            | xh    | primary |

        When I route I should get
            | from | to | route    | time   |
            | x    | a  | xa,xa    | 1s +-1 |
            | x    | b  | xb,xb    | 1s +-1 |
            | x    | c  | xc,xc    | 1s +-1 |
            | x    | d  | xd,xd    | 1s +-1 |
            | x    | e  | xe,xe    | 1s +-1 |
            | x    | f  | xf,xf    | 1s +-1 |
            | x    | g  | xg,xg    | 1s +-1 |
            | x    | h  | xh,xh    | 1s +-1 |

    Scenario: Basic travel time, 100m scale
        Given a grid size of 100 meters
        Given the node map
            """
            h a b
            g x c
            f e d
            """

        And the ways
            | nodes | highway |
            | xa    | primary |
            | xb    | primary |
            | xc    | primary |
            | xd    | primary |
            | xe    | primary |
            | xf    | primary |
            | xg    | primary |
            | xh    | primary |

        When I route I should get
            | from | to | route    | time    |
            | x    | a  | xa,xa    | 10s +-1 |
            | x    | b  | xb,xb    | 14s +-1 |
            | x    | c  | xc,xc    | 10s +-1 |
            | x    | d  | xd,xd    | 14s +-1 |
            | x    | e  | xe,xe    | 10s +-1 |
            | x    | f  | xf,xf    | 14s +-1 |
            | x    | g  | xg,xg    | 10s +-1 |
            | x    | h  | xh,xh    | 14s +-1 |

    Scenario: Basic travel time, 1km scale
        Given a grid size of 1000 meters
        Given the node map
            """
            h a b
            g x c
            f e d
            """

        And the ways
            | nodes | highway |
            | xa    | primary |
            | xb    | primary |
            | xc    | primary |
            | xd    | primary |
            | xe    | primary |
            | xf    | primary |
            | xg    | primary |
            | xh    | primary |

        When I route I should get
            | from | to | route    | time     |
            | x    | a  | xa,xa    | 100s +-1 |
            | x    | b  | xb,xb    | 141s +-1 |
            | x    | c  | xc,xc    | 100s +-1 |
            | x    | d  | xd,xd    | 141s +-1 |
            | x    | e  | xe,xe    | 100s +-1 |
            | x    | f  | xf,xf    | 141s +-1 |
            | x    | g  | xg,xg    | 100s +-1 |
            | x    | h  | xh,xh    | 141s +-1 |

    Scenario: Basic travel time, 10km scale
        Given a grid size of 10000 meters
        Given the node map
            """
            h a b
            g x c
            f e d
            """

        And the ways
            | nodes | highway |
            | xa    | primary |
            | xb    | primary |
            | xc    | primary |
            | xd    | primary |
            | xe    | primary |
            | xf    | primary |
            | xg    | primary |
            | xh    | primary |

        When I route I should get
            | from | to | route    | time      |
            | x    | a  | xa,xa    | 1000s +-1 |
            | x    | b  | xb,xb    | 1414s +-1 |
            | x    | c  | xc,xc    | 1000s +-1 |
            | x    | d  | xd,xd    | 1414s +-1 |
            | x    | e  | xe,xe    | 1000s +-1 |
            | x    | f  | xf,xf    | 1414s +-1 |
            | x    | g  | xg,xg    | 1000s +-1 |
            | x    | h  | xh,xh    | 1414s +-1 |

    Scenario: Time of travel depending on way type
        Given the node map
            """
            a b
            c d
            e f
            """

        And the ways
            | nodes | highway   |
            | ab    | primary   |
            | cd    | secondary |
            | ef    | tertiary  |
            | ace   | something |

        When I route I should get
            | from | to | route    | time    |
            | a    | b  | ab,ab    | 10s +-1 |
            | c    | d  | cd,cd    | 20s +-1 |
            | e    | f  | ef,ef    | 30s +-1 |

    Scenario: Time of travel on a series of ways
        Given the node map
            """
            a b e
            f c d
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | cd    | primary |
            | be    | primary |
            | cf    | primary |

        When I route I should get
            | from | to | route       | time    |
            | a    | b  | ab,ab       | 10s +-1 |
            | a    | c  | ab,bc,bc    | 20s +-1 |
            | a    | d  | ab,bc,cd,cd | 30s +-1 |

    Scenario: Time of travel on a winding way
        Given the node map
            """
            a   i h
            b c   g
              d e f
            """

        And the ways
            | nodes     | highway |
            | abcdefghi | primary |

        When I route I should get
            | from | to | route               | time    |
            | a    | b  | abcdefghi,abcdefghi | 10s +-1 |
            | a    | e  | abcdefghi,abcdefghi | 40s +-1 |
            | a    | i  | abcdefghi,abcdefghi | 80s +-1 |

    Scenario: Time of travel on combination of road types
        Given the node map
            """
            a b c
                d
                e
            """

        And the ways
            | nodes | highway  |
            | abc   | primary  |
            | cde   | tertiary |

        When I route I should get
            | from | to | route       | time    |
            | b    | c  | abc,abc     | 10s +-1 |
            | c    | e  | cde,cde     | 60s +-1 |
            | b    | d  | abc,cde     | 40s +-1 |
            | a    | e  | abc,cde,cde | 80s +-1 |

    Scenario: Time of travel on part of a way
        Given the node map
            """
            a 1
              2
              3
            b 4
            """

        And the ways
            | nodes | highway |
            | ab    | primary |

        When I route I should get
            | from | to | route    | time    |
            | 1    | 2  | ab,ab    | 10s +-1 |
            | 1    | 3  | ab,ab    | 20s +-1 |
            | 1    | 4  | ab,ab    | 30s +-1 |
            | 4    | 3  | ab,ab    | 10s +-1 |
            | 4    | 2  | ab,ab    | 20s +-1 |
            | 4    | 1  | ab,ab    | 30s +-1 |

    Scenario: Total travel time should match sum of times of individual ways
        Given a grid size of 1000 meters
        And the node map
            """
            a b

              c     d
            """

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | cd    | primary |

        When I route I should get
            | from | to | route       | distances             | distance  | times              | time     |
            | a    | b  | ab,ab       | 1000m +-1             | 1000m +-1 | 100s +-1           | 100s +-1 |
            | b    | c  | bc,bc       | 2000m +-1             | 2000m +-1 | 200s +-1           | 200s +-1 |
            | c    | d  | cd,cd       | 3000m +-1             | 3000m +-1 | 300s +-1           | 300s +-1 |
            | a    | c  | ab,bc,bc    | 1000m,2000m +-1       | 3000m +-1 | 100s,200s +-1      | 300s +-1 |
            | b    | d  | bc,cd,cd    | 2000m,3000m +-1       | 5000m +-1 | 200s,300s +-1      | 500s +-1 |
            | a    | d  | ab,bc,cd,cd | 1000m,2000m,3000m +-1 | 6000m +-1 | 100s,200s,300s +-1 | 600s +-1 |
