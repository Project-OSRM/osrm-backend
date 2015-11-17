@routing @bearing_param @testbot
Feature: Bearing parameter

    Background:
        Given the profile "testbot"
        And a grid size of 10 meters

    Scenario: Testbot - Intial bearing in simple case
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes |
            | ad    |

        When I route I should get
            | from | to | param:b     | route | bearing |
            | b    | c  | 90&b=90     | ad    | 90      |
            | b    | c  | 180&b=90    |       |         |
            | b    | c  | 80&b=100    | ad    | 90      |
            | b    | c  | 79&b=100    |       |         |
            | b    | c  | 79,11&b=100 | ad    | 90      |

    Scenario: Testbot - Intial bearing in simple case
        Given the node map
            | a |   |
            | 0 | c |
            | b |   |

        And the ways
            | nodes |
            | ac    |
            | bc    |

        When I route I should get
            | from | to | param:b    | route | bearing |
            | 0    | c  | 0&b=0      |       |         |
            | 0    | c  | 45&b=45    | bc    | 45 ~3%   |
            | 0    | c  | 85&b=85    |       |         |
            | 0    | c  | 95&b=95    |       |         |
            | 0    | c  | 135&b=135  | ac    | 135 ~1%  |
            | 0    | c  | 180&b=180  |       |         |

    Scenario: Testbot - Initial bearing on split way
        Given the node map
        | d |  |  |  |  | 1 |  |  |  |  | c |
        | a |  |  |  |  | 0 |  |  |  |  | b |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | from | to | param:b    | route    | bearing  |
            | 0    | b  | 10&b=10    |          |          |
            | 0    | b  | 90&b=90    | ab       | 90       |
            | 0    | b  | 170&b=170  |          |          |
            | 0    | b  | 190&b=190  |          |          |
            | 0    | 1  | 90&b=270   | ab,bc,cd | 90,0,270 |
            | 1    | d  | 10&b=10    |          |          |
            | 1    | d  | 90&b=90    |          |          |
            | 1    | 0  | 190&b=190  |          |          |
            | 1    | d  | 270&b=270  | cd       | 270      |
            | 1    | d  | 350&b=350  |          |          |

    Scenario: Testbot - Initial bearing in all direction
        Given the node map
            | h |  | q | a |   |  | b |
            |   |  |   |   |   |  |   |
            |   |  | p | i | j |  |   |
            | g |  | o | 0 | k |  | c |
            |   |  | n | m | l |  |   |
            |   |  |   |   |   |  |   |
            | f |  |   | e |   |  | d |

        And the ways
            | nodes | oneway |
            | ia    | yes    |
            | jb    | yes    |
            | kc    | yes    |
            | ld    | yes    |
            | me    | yes    |
            | nf    | yes    |
            | og    | yes    |
            | ph    | yes    |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fg    | yes    |
            | gh    | yes    |
            | ha    | yes    |

        When I route I should get
            | from | to | param:b     | route                      | bearing                     |
            | i    | q  | 0&b=90      | ia,ab,bc,cd,de,ef,fg,gh,ha | 0,90,180,180,270,270,0,0,90 |
            | 0    | a  | 45&b=90     | jb,bc,cd,de,ef,fg,gh,ha    | 45,180,180,270,270,0,0,90   |
            | j    | q  | 90&b=90     | kc,cd,de,ef,fg,gh,ha       | 90,180,270,270,0,0,90       |
            | k    | a  | 135&b=90    | ld,de,ef,fg,gh,ha          | 135,270,270,0,0,90          |
            | 0    | a  | 180&b=90    | me,ef,fg,gh,ha             | 180,270,0,0,90              |
            | m    | a  | 225&b=90    | nf,fg,gh,ha                | 225,0,0,90                  |
            | 0    | a  | 270&b=90    | og,gh,ha                   | 270,0,90                    |
            | 0    | a  | 315&b=90    | ph,ha                      | 315,90                      |
