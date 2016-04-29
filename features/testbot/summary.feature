@routing @basic @testbot
Feature: Basic Routing

    Background:
        Given the profile "testbot"

    @smallest
    Scenario: Checking 
        Given the node map
            | a | b |  | c | d | e |

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

    @smallest
    Scenario: Check handling empty values
        Given the node map
            | a | b |  | c | d | e | f |

        And the ways
            | nodes | name |
            | ab    | ab   |
            | bc    | bc   |
            | cd    | cd   |
            | de    | de   |
            | ef    |      |

        When I route I should get
            | from | to | route             | summary  |
            | f    | a  | ,de,cd,bc,ab,ab   | de, bc   |

    @smallest @todo
    Scenario: Summaries when routing on a simple network
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | summary  |
            | a    | b  | ab,ab | ab       |
            | b    | a  | ab,ab | ab       |

    @repeated
    Scenario: Check handling empty values
        Given the node map
            | f |   |   |   |
            | b | c | d | e |
            | a |   |   |   |

        And the ways
            | nodes | name   |
            | ab    | first  |
            | bc    | first  |
            | cd    | first  |
            | de    | second |
            | bf    | third  |

        When I route I should get
            | from | to | route               | summary       |
            | a    | e  | first,second,second | first, second |

