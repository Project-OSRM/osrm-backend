@routing @testbot @oneway
Feature: Testbot - oneways

    Background:
        Given the profile "testbot"
        Given a grid size of 250 meters

    Scenario: Routing on a oneway roundabout
        Given the node map
        """
                v
        x   d c
          e     b
          f     a
            g h   y
          z
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
            | vx    | yes    |
            | vy    | yes    |
            | yz    | yes    |
            | xe    | yes    |

        When I route I should get
            | from | to | route                   |
            | a    | b  | ab,ab                   |
            | b    | c  | bc,bc                   |
            | c    | d  | cd,cd                   |
            | d    | e  | de,de                   |
            | e    | f  | ef,ef                   |
            | f    | g  | fg,fg                   |
            | g    | h  | gh,gh                   |
            | h    | a  | ha,ha                   |
            | b    | a  | bc,cd,de,ef,fg,gh,ha,ha |
            | c    | b  | cd,de,ef,fg,gh,ha,ab,ab |
            | d    | c  | de,ef,fg,gh,ha,ab,bc,bc |
            | e    | d  | ef,fg,gh,ha,ab,bc,cd,cd |
            | f    | e  | fg,gh,ha,ab,bc,cd,de,de |
            | g    | f  | gh,ha,ab,bc,cd,de,ef,ef |
            | h    | g  | ha,ab,bc,cd,de,ef,fg,fg |
            | a    | h  | ab,bc,cd,de,ef,fg,gh,gh |

    Scenario: Testbot - Simple oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | yes    | x    |       |

    Scenario: Simple reverse oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | -1     |      | x     |

    Scenario: Testbot - Around the Block
        Given the node map
            """
              a b
            e d c f
            """

        And the ways
            | nodes | oneway | foot |
            | ab    | yes    | no   |
            | bc    |        | no   |
            | cd    |        | no   |
            | da    |        | no   |
            | de    |        | no   |
            | cf    |        | no   |

        When I route I should get
            | from | to | route       |
            | a    | b  | ab,ab       |
            | b    | a  | bc,cd,da,da |

    Scenario: Testbot - Handle various oneway tag values
        Then routability should be
            | foot | oneway   | forw | backw |
            | no   |          | x    | x     |
            | no   | nonsense | x    | x     |
            | no   | no       | x    | x     |
            | no   | false    | x    | x     |
            | no   | 0        | x    | x     |
            | no   | yes      | x    |       |
            | no   | true     | x    |       |
            | no   | 1        | x    |       |
            | no   | -1       |      | x     |

    Scenario: Testbot - Two consecutive oneways
        Given the node map
            """
            a b c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |


        When I route I should get
            | from | to | route    |
            | a    | c  | ab,bc,bc |
