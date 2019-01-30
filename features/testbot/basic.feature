@routing @basic @testbot
Feature: Basic Routing

    Background:
        Given the profile "testbot"
        Given a grid size of 100 meters

    @smallest
    Scenario: A single way with two nodes
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route    | data_version |
            | a    | b  | ab,ab    |              |
            | b    | a  | ab,ab    |              |

    Scenario: Data_version test
        Given the node map
            """
            a b
            """

        And the extract extra arguments "--data_version cucumber_data_version"

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route    | data_version          |
            | a    | b  | ab,ab    | cucumber_data_version |
            | b    | a  | ab,ab    | cucumber_data_version |

    Scenario: Routing in between two nodes of way
        Given the node map
            """
            a b 1 2 c d
            """

        And the ways
            | nodes |
            | abcd  |

        When I route I should get
            | from | to | route      |
            | 1    | 2  | abcd,abcd  |
            | 2    | 1  | abcd,abcd  |

    Scenario: Routing between the middle nodes of way
        Given the node map
            """
            a b c d e f
            """

        And the ways
            | nodes  |
            | abcdef |

        When I route I should get
            | from | to | route         |
            | b    | c  | abcdef,abcdef |
            | b    | d  | abcdef,abcdef |
            | b    | e  | abcdef,abcdef |
            | c    | b  | abcdef,abcdef |
            | c    | d  | abcdef,abcdef |
            | c    | e  | abcdef,abcdef |
            | d    | b  | abcdef,abcdef |
            | d    | c  | abcdef,abcdef |
            | d    | e  | abcdef,abcdef |
            | e    | b  | abcdef,abcdef |
            | e    | c  | abcdef,abcdef |
            | e    | d  | abcdef,abcdef |

    Scenario: Two ways connected in a straight line
        Given the node map
            """
            a   b   c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | route    |
            | a    | c  | ab,bc,bc |
            | c    | a  | bc,ab,ab |
            | a    | b  | ab,ab    |
            | b    | a  | ab,ab    |
            | b    | c  | bc,bc    |
            | c    | b  | bc,bc    |

    Scenario: 2 unconnected parallel ways
        Given the node map
            """
            a b c
            d e f
            """

        And the ways
            | nodes |
            | abc   |
            | def   |

        When I route I should get
            | from | to | route   |
            | a    | b  | abc,abc |
            | b    | a  | abc,abc |
            | b    | c  | abc,abc |
            | c    | b  | abc,abc |
            | d    | e  | def,def |
            | e    | d  | def,def |
            | e    | f  | def,def |
            | f    | e  | def,def |
            | a    | d  |         |
            | d    | a  |         |
            | b    | d  |         |
            | d    | b  |         |
            | c    | d  |         |
            | d    | c  |         |
            | a    | e  |         |
            | e    | a  |         |
            | b    | e  |         |
            | e    | b  |         |
            | c    | e  |         |
            | e    | c  |         |
            | a    | f  |         |
            | f    | a  |         |
            | b    | f  |         |
            | f    | b  |         |
            | c    | f  |         |
            | f    | c  |         |

    Scenario: 3 ways connected in a triangle
        Given the node map
            """
            a   b

              c
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | ca    |

        When I route I should get
            | from | to | route  |
            | a    | b  | ab,ab  |
            | a    | c  | ca,ca  |
            | b    | c  | bc,bc  |
            | b    | a  | ab,ab  |
            | c    | a  | ca,ca  |
            | c    | b  | bc,bc  |

    Scenario: 3 connected triangles
        Given the node map
            """
            x a   b s
            y       t
                c
              v   w
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | ca    |
            | ax    |
            | xy    |
            | ya    |
            | bs    |
            | st    |
            | tb    |
            | cv    |
            | vw    |
            | wc    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab,ab |
            | a    | c  | ca,ca |
            | b    | c  | bc,bc |
            | b    | a  | ab,ab |
            | c    | a  | ca,ca |
            | c    | b  | bc,bc |

    Scenario: To ways connected at a 90 degree angle
        Given the node map
            """
            a
            |
            b
            |
            c----d----e
            """

        And the ways
            | nodes |
            | abc   |
            | cde   |

        When I route I should get
            | from | to | route       |
            | b    | d  | abc,cde,cde |
            | a    | e  | abc,cde,cde |
            | a    | c  | abc,abc     |
            | c    | a  | abc,abc     |
            | c    | e  | cde,cde     |
            | e    | c  | cde,cde     |

    Scenario: Grid city center
        Given the node map
            """
            a b c d
            e f g h
            i j k l
            m n o p
            """

        And the ways
            | nodes |
            | abcd  |
            | efgh  |
            | ijkl  |
            | mnop  |
            | aeim  |
            | bfjn  |
            | cgko  |
            | dhlp  |

        When I route I should get
            | from | to | route     |
            | f    | g  | efgh,efgh |
            | g    | f  | efgh,efgh |
            | f    | j  | bfjn,bfjn |
            | j    | f  | bfjn,bfjn |

    Scenario: Grid city periphery
        Given the node map
            """
            a b c d
            e f g h
            i j k l
            m n o p
            """

        And the ways
            | nodes |
            | abcd  |
            | efgh  |
            | ijkl  |
            | mnop  |
            | aeim  |
            | bfjn  |
            | cgko  |
            | dhlp  |

        When I route I should get
            | from | to | route      |
            | a    | d  | abcd,abcd  |
            | d    | a  | abcd,abcd  |
            | a    | m  | aeim,aeim  |
            | m    | a  | aeim,aeim  |

    Scenario: Testbot - Triangle challenge
        Given the node map
            """
                  d
            a b c
                  e
            """

        And the ways
            | nodes | highway | oneway |
            | abc   | primary |        |
            | cd    | primary | yes    |
            | ce    | river   |        |
            | de    | primary |        |

        When I route I should get
            | from | to | route    |
            | d    | c  | de,ce,ce |
            | e    | d  | de,de    |

    Scenario: Ambiguous edge weights - Use minimal edge weight
        Given the node map
            """
            a b
            """

        And the ways
            | nodes | highway   | name |
            | ab    | tertiary  |      |
            | ab    | primary   |      |
            | ab    | secondary |      |

        When I route I should get
            | from | to | route | time |
            | a    | b  | ,     | 10s  |
            | b    | a  | ,     | 10s  |

    Scenario: Ambiguous edge names - Use lexicographically smallest name
        Given the node map
            """
            a-------b-------c
            """

        And the ways
            | nodes | highway | name |
            | ab    | primary |      |
            | ab    | primary | Αβγ  |
            | ab    | primary |      |
            | ab    | primary | Abc  |
            | ab    | primary |      |
            | ab    | primary | Абв  |
            | bc    | primary | Ηθι  |
            | bc    | primary | Δεζ  |
            | bc    | primary | Где  |

        When I route I should get
            | from | to | route       |
            | a    | c  | Abc,Δεζ,Δεζ |
            | c    | a  | Δεζ,Abc,Abc |
