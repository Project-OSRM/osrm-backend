@routing @snap @testbot
Feature: Snap start/end point to the nearest way

    Background:
        Given the profile "testbot"

    Scenario: Snap to nearest protruding oneway
        Given the node map
            """
              1   2
            8   n   3
              w c e
            7   s   4
              6   5
            """

        And the ways
            | nodes |
            | nc    |
            | ec    |
            | sc    |
            | wc    |

        When I route I should get
            | from | to | route |
            | 1    | c  | nc,nc |
            | 2    | c  | nc,nc |
            | 3    | c  | ec,ec |
            | 4    | c  | ec,ec |
            | 5    | c  | sc,sc |
            | 6    | c  | sc,sc |
            | 7    | c  | wc,wc |
            | 8    | c  | wc,wc |

    Scenario: Snap to nearest edge of a square
        Given the node map
            """
            4 5 6 7
            3 a   u
            2
            1 d   b
            """

        And the ways
            | nodes |
            | aub   |
            | adb   |

        When I route I should get
            | from | to | route   |
            | 1    | b  | adb,adb |
            | 2    | b  | adb,adb |
            | 6    | b  | aub,aub |
            | 7    | b  | aub,aub |

    Scenario: Snap to edge right under start/end point
        Given the node map
            """
            d   e   f   g

            c           h

            b           i

            a   l   k   j
            """

        And the ways
            | nodes |
            | abcd  |
            | defg  |
            | ghij  |
            | jkla  |

        When I route I should get
            | from | to | route          |
            | a    | b  | abcd,abcd      |
            | a    | c  | abcd,abcd      |
            | a    | d  | abcd,abcd      |
            | a    | e  | abcd,defg,defg |
            | a    | f  | abcd,defg,defg |
            | a    | h  | jkla,ghij,ghij |
            | a    | i  | jkla,ghij,ghij |
            | a    | j  | jkla,jkla      |
            | a    | k  | jkla,jkla      |
            | a    | l  | jkla,jkla      |

    Scenario: Snapping in viaroute
        Given the extract extra arguments "--small-component-size 4"
        Given the node map
            """
            a   c e
            b   d f
            """

        And the ways
            | nodes |
            | ab    |
            | cd    |
            | df    |
            | fe    |
            | ec    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab,ab |
            | a    | d  | cd,cd |
            | c    | d  | cd,cd |

    Scenario: Snap to correct way at large scales
        Given a grid size of 1000 meters
        Given the node map
            """
                  a
            x     b
                  c
            """

        And the ways
            | nodes |
            | xa    |
            | xb    |
            | xc    |

        When I route I should get
            | from | to | route |
            | x    | a  | xa,xa |
            | x    | b  | xb,xb |
            | x    | c  | xc,xc |
            | a    | x  | xa,xa |
            | b    | x  | xb,xb |
            | c    | x  | xc,xc |

    Scenario: Find edges within 100m, and the same from 1km
        Given a grid size of 100 meters
        Given the node map
            """
            p               i               j





                        8   1   2
                          h a b
            o           7 g x c 3           k
                          f e d
                        6   5   4





            n               m               l
            """

        Given the ways
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
            | from | to | route |
            | x    | 1  | xa,xa |
            | x    | 2  | xb,xb |
            | x    | 3  | xc,xc |
            | x    | 4  | xd,xd |
            | x    | 5  | xe,xe |
            | x    | 6  | xf,xf |
            | x    | 7  | xg,xg |
            | x    | 8  | xh,xh |
            | x    | i  | xa,xa |
            | x    | j  | xb,xb |
            | x    | k  | xc,xc |
            | x    | l  | xd,xd |
            | x    | m  | xe,xe |
            | x    | n  | xf,xf |
            | x    | o  | xg,xg |
            | x    | p  | xh,xh |