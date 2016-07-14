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
            | from | to | bearings  | route | bearing    |
            | b    | c  | 90 90     | ad,ad | 0->90,90->0|
            | b    | c  | 180 90    |       |            |
            | b    | c  | 80 100    | ad,ad | 0->90,90->0|
            | b    | c  | 79 100    |       |            |
            | b    | c  | 79,11 100 | ad,ad | 0->90,90->0|

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
            | from | to | bearings | route | bearing           |
            | 0    | c  | 0 0      |       |                   |
            | 0    | c  | 45 45    | bc,bc | 0->44,44->0 +- 1  |
            | 0    | c  | 85 85    |       |                   |
            | 0    | c  | 95 95    |       |                   |
            | 0    | c  | 135 135  | ac,ac | 0->135,135->0 +- 1|
            | 0    | c  | 180 180  |       |                   |

    Scenario: Testbot - Initial bearing on split way
        Given the node map
           | g | d |  |  |  |  | 1 |  |  |  |  | c | f |
           | h | a |  |  |  |  | 0 |  |  |  |  | b | e |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |
            | be    | yes    |
            | fc    | yes    |
            | dg    | yes    |
            | ha    | yes    |

        When I route I should get
            | from | to | bearings | route          | bearing                             |
            | 0    | b  | 10 10    | bc,bc          | 0->0,0->0                           |
            | 0    | b  | 90 90    | ab,ab          | 0->90,90->0                         |
            # The returned bearing is wrong here, it's based on the snapped
            # coordinates, not the acutal edge bearing.  This should be
            # fixed one day, but it's only a problem when we snap two vias
            # to the same point - DP
            #| 0    | b  | 170 170  | da          | 180                                   |
            #| 0    | b  | 189 189  | da          | 180                                   |
            | 0    | 1  | 90 270   | ab,bc,cd,cd    | 0->90,90->0,0->270,270->0           |
            | 1    | d  | 10 10    | bc,bc          | 0->0,0->0                           |
            | 1    | d  | 90 90    | ab,bc,cd,da,da | 0->90,90->0,0->270,270->180,180->0  |
            | 1    | 0  | 189 189  | da,da          | 0->180,180->0                       |
            | 1    | d  | 270 270  | cd,cd          | 0->270,270->0                       |
            | 1    | d  | 349 349  |                |                                     |

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
            | nodes | oneway | name |
            | ia    | yes    | ia   |
            | jb    | yes    | jb   |
            | kc    | yes    | kc   |
            | ld    | yes    | ld   |
            | me    | yes    | me   |
            | nf    | yes    | nf   |
            | og    | yes    | og   |
            | ph    | yes    | ph   |
            | ab    | yes    | ring |
            | bc    | yes    | ring |
            | cd    | yes    | ring |
            | de    | yes    | ring |
            | ef    | yes    | ring |
            | fg    | yes    | ring |
            | gh    | yes    | ring |
            | ha    | yes    | ring |

        When I route I should get
            | from | to | bearings | route        | bearing               |
            | 0    | q  | 0 90     | ia,ring,ring | 0->0,0->90,90->0      |
            | 0    | a  | 45 90    | jb,ring,ring | 0->45,45->180,90->0   |
            | 0    | q  | 90 90    | kc,ring,ring | 0->90,90->180,90->0   |
            | 0    | a  | 135 90   | ld,ring,ring | 0->135,135->270,90->0 |
            | 0    | a  | 180 90   | me,ring,ring | 0->180,180->270,90->0 |
            | 0    | a  | 225 90   | nf,ring,ring | 0->225,225->0,90->0   |
            | 0    | a  | 270 90   | og,ring,ring | 0->270,270->0,90->0   |
            | 0    | a  | 315 90   | ph,ring,ring | 0->315,315->90,90->0  |
