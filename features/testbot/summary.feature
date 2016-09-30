@routing @basic @testbot
Feature: Basic Routing

    Background:
        Given the profile "testbot"
        Given a grid size of 200 meters

    @smallest
    Scenario: Checking
        Given the node map
            """
            a b 1 c d e
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | de    |

        When I route I should get
            | from | to | route          | summary  |
            | a    | e  | ab,bc,cd,de,de | ab, bc   |
            | e    | a  | de,cd,bc,ab,ab | de, bc   |
            | a    | b  | ab,ab          | ab       |
            | b    | d  | bc,cd,cd       | bc, cd   |
            | 1    | c  | bc,bc          | bc       |

    @smallest
    Scenario: Check handling empty values
        Given the node map
            """
            a b   c   d f
                      e
            """

        And the ways
            | nodes | name |
            | ab    | ab   |
            | bc    | bc   |
            | cd    |      |
            | de    | de   |
            | df    | df   |

        When I route I should get
            | from | to | route          | summary  |
            | e    | a  | de,,bc,ab,ab   | de, bc   |

    @smallest @todo
    Scenario: Summaries when routing on a simple network
        Given the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | summary |
            | a    | b  | ab,ab | ab      |
            | b    | a  | ab,ab | ab      |

    @repeated
    Scenario: Check handling empty values
        Given the node map
            """
            f     x
            b c d e 1 g
            a     y
            """

        And the ways
            | nodes | name   | # |
            | ab    | first  |   |
            | bc    | first  |   |
            | cd    | first  |   |
            | deg   | second |   |
            | bf    | third  |   |
            | xey   | cross  |we need this because phantom node segments are not considered for the summary |

        When I route I should get
            | from | to | route                     | summary       |
            | a    | 1  | first,first,second,second | first, second |

