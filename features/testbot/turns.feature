@routing @turns @testbot
Feature: Turn directions/codes

    Background:
        Given the profile "testbot"

    Scenario: Turn directions
        Given the node map
            | o | p | a | b | c |
            | n |   |   |   | d |
            | m |   | x |   | e |
            | l |   |   |   | f |
            | k | j | i | h | g |

        And the ways
            | nodes |
            | xi    |
            | xk    |
            | xm    |
            | xo    |
            | xa    |
            | xc    |
            | xe    |
            | xg    |

        When I route I should get
            | from | to | route    | turns                      |
            | i    | k  | xi,xk,xk | depart,sharp left,arrive   |
            | i    | m  | xi,xm,xm | depart,left,arrive         |
            | i    | o  | xi,xo,xo | depart,slight left,arrive  |
            | i    | a  | xi,xa,xa | depart,straight,arrive     |
            | i    | c  | xi,xc,xc | depart,slight right,arrive |
            | i    | e  | xi,xe,xe | depart,right,arrive        |
            | i    | g  | xi,xg,xg | depart,sharp right,arrive  |

            | k | m | xk,xm,xm | depart,sharp left,arrive   |
            | k | o | xk,xo,xo | depart,left,arrive         |
            | k | a | xk,xa,xa | depart,slight left,arrive  |
            | k | c | xk,xc,xc | depart,straight,arrive     |
            | k | e | xk,xe,xe | depart,slight right,arrive |
            | k | g | xk,xg,xg | depart,right,arrive        |
            | k | i | xk,xi,xi | depart,sharp right,arrive  |

            | m | o | xm,xo,xo | depart,sharp left,arrive   |
            | m | a | xm,xa,xa | depart,left,arrive         |
            | m | c | xm,xc,xc | depart,slight left,arrive  |
            | m | e | xm,xe,xe | depart,straight,arrive     |
            | m | g | xm,xg,xg | depart,slight right,arrive |
            | m | i | xm,xi,xi | depart,right,arrive        |
            | m | k | xm,xk,xk | depart,sharp right,arrive  |

            | o | a | xo,xa,xa | depart,sharp left,arrive   |
            | o | c | xo,xc,xc | depart,left,arrive         |
            | o | e | xo,xe,xe | depart,slight left,arrive  |
            | o | g | xo,xg,xg | depart,straight,arrive     |
            | o | i | xo,xi,xi | depart,slight right,arrive |
            | o | k | xo,xk,xk | depart,right,arrive        |
            | o | m | xo,xm,xm | depart,sharp right,arrive  |

            | a | c | xa,xc,xc | depart,sharp left,arrive   |
            | a | e | xa,xe,xe | depart,left,arrive         |
            | a | g | xa,xg,xg | depart,slight left,arrive  |
            | a | i | xa,xi,xi | depart,straight,arrive     |
            | a | k | xa,xk,xk | depart,slight right,arrive |
            | a | m | xa,xm,xm | depart,right,arrive        |
            | a | o | xa,xo,xo | depart,sharp right,arrive  |

            | c | e | xc,xe,xe | depart,sharp left,arrive   |
            | c | g | xc,xg,xg | depart,left,arrive         |
            | c | i | xc,xi,xi | depart,slight left,arrive  |
            | c | k | xc,xk,xk | depart,straight,arrive     |
            | c | m | xc,xm,xm | depart,slight right,arrive |
            | c | o | xc,xo,xo | depart,right,arrive        |
            | c | a | xc,xa,xa | depart,sharp right,arrive  |

            | e | g | xe,xg,xg | depart,sharp left,arrive   |
            | e | i | xe,xi,xi | depart,left,arrive         |
            | e | k | xe,xk,xk | depart,slight left,arrive  |
            | e | m | xe,xm,xm | depart,straight,arrive     |
            | e | o | xe,xo,xo | depart,slight right,arrive |
            | e | a | xe,xa,xa | depart,right,arrive        |
            | e | c | xe,xc,xc | depart,sharp right,arrive  |

            | g | i | xg,xi,xi | depart,sharp left,arrive   |
            | g | k | xg,xk,xk | depart,left,arrive         |
            | g | m | xg,xm,xm | depart,slight left,arrive  |
            | g | o | xg,xo,xo | depart,straight,arrive     |
            | g | a | xg,xa,xa | depart,slight right,arrive |
            | g | c | xg,xc,xc | depart,right,arrive        |
            | g | e | xg,xe,xe | depart,sharp right,arrive  |

    Scenario: Turn instructions at high latitude
    # https://github.com/DennisOSRM/Project-OSRM/issues/532
        Given the node locations
            | node | lat       | lon      |
            | a    | 55.68740  | 12.52430 |
            | b    | 55.68745  | 12.52409 |
            | c    | 55.68711  | 12.52383 |
            | x    | -55.68740 | 12.52430 |
            | y    | -55.68745 | 12.52409 |
            | z    | -55.68711 | 12.52383 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | xy    |
            | yz    |

        When I route I should get
            | from | to | route    | turns                  |
            | a    | c  | ab,bc,bc | depart,straight,arrive |
            | c    | a  | bc,ab,ab | depart,straight,arrive |
            | x    | z  | xy,yz,yz | depart,straight,arrive |
            | z    | x  | yz,xy,xy | depart,straight,arrive |
