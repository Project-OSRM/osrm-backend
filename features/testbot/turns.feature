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
            | from | to | route | turns                      |
            | i    | k  | xi,xk | depart,sharp_left,arrive   |
            | i    | m  | xi,xm | depart,left,arrive         |
            | i    | o  | xi,xo | depart,slight_left,arrive  |
            | i    | a  | xi,xa | depart,straight,arrive     |
            | i    | c  | xi,xc | depart,slight_right,arrive |
            | i    | e  | xi,xe | depart,right,arrive        |
            | i    | g  | xi,xg | depart,sharp_right,arrive  |

            | k | m | xk,xm | depart,sharp_left,arrive   |
            | k | o | xk,xo | depart,left,arrive         |
            | k | a | xk,xa | depart,slight_left,arrive  |
            | k | c | xk,xc | depart,straight,arrive     |
            | k | e | xk,xe | depart,slight_right,arrive |
            | k | g | xk,xg | depart,right,arrive        |
            | k | i | xk,xi | depart,sharp_right,arrive  |

            | m | o | xm,xo | depart,sharp_left,arrive   |
            | m | a | xm,xa | depart,left,arrive         |
            | m | c | xm,xc | depart,slight_left,arrive  |
            | m | e | xm,xe | depart,straight,arrive     |
            | m | g | xm,xg | depart,slight_right,arrive |
            | m | i | xm,xi | depart,right,arrive        |
            | m | k | xm,xk | depart,sharp_right,arrive  |

            | o | a | xo,xa | depart,sharp_left,arrive   |
            | o | c | xo,xc | depart,left,arrive         |
            | o | e | xo,xe | depart,slight_left,arrive  |
            | o | g | xo,xg | depart,straight,arrive     |
            | o | i | xo,xi | depart,slight_right,arrive |
            | o | k | xo,xk | depart,right,arrive        |
            | o | m | xo,xm | depart,sharp_right,arrive  |

            | a | c | xa,xc | depart,sharp_left,arrive   |
            | a | e | xa,xe | depart,left,arrive         |
            | a | g | xa,xg | depart,slight_left,arrive  |
            | a | i | xa,xi | depart,straight,arrive     |
            | a | k | xa,xk | depart,slight_right,arrive |
            | a | m | xa,xm | depart,right,arrive        |
            | a | o | xa,xo | depart,sharp_right,arrive  |

            | c | e | xc,xe | depart,sharp_left,arrive   |
            | c | g | xc,xg | depart,left,arrive         |
            | c | i | xc,xi | depart,slight_left,arrive  |
            | c | k | xc,xk | depart,straight,arrive     |
            | c | m | xc,xm | depart,slight_right,arrive |
            | c | o | xc,xo | depart,right,arrive        |
            | c | a | xc,xa | depart,sharp_right,arrive  |

            | e | g | xe,xg | depart,sharp_left,arrive   |
            | e | i | xe,xi | depart,left,arrive         |
            | e | k | xe,xk | depart,slight_left,arrive  |
            | e | m | xe,xm | depart,straight,arrive     |
            | e | o | xe,xo | depart,slight_right,arrive |
            | e | a | xe,xa | depart,right,arrive        |
            | e | c | xe,xc | depart,sharp_right,arrive  |

            | g | i | xg,xi | depart,sharp_left,arrive   |
            | g | k | xg,xk | depart,left,arrive         |
            | g | m | xg,xm | depart,slight_left,arrive  |
            | g | o | xg,xo | depart,straight,arrive     |
            | g | a | xg,xa | depart,slight_right,arrive |
            | g | c | xg,xc | depart,right,arrive        |
            | g | e | xg,xe | depart,sharp_right,arrive  |

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
            | from | to | route | turns                  |
            | a    | c  | ab,bc | depart,straight,arrive |
            | c    | a  | bc,ab | depart,straight,arrive |
            | x    | z  | xy,yz | depart,straight,arrive |
            | z    | x  | yz,xy | depart,straight,arrive |
