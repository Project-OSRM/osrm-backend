@routing @turns @testbot
Feature: Turn directions/codes

    Background:
        Given the profile "testbot"

    Scenario: Turn directions
        Given the query options
            | instructions | true |
        Given the node map
            | o | p | a | b | c |
            | n |   |   |   | d |
            | m |   | x |   | e |
            | l |   |   |   | f |
            | k | j | i | h | g |

        And the ways
            | nodes |
            | xa    |
            | xc    |
            | xe    |
            | xg    |
            | xi    |
            | xk    |
            | xm    |
            | xo    |

        When I match I should get
            | trace | route    | turns                      | matchings |
            | im    | xi,xm,xm | depart,left,arrive         | im        |
            | io    | xi,xo,xo | depart,slight_left,arrive  | io        |
            | ia    | xi,xa,xa | depart,straight,arrive     | ia        |
            | ic    | xi,xc,xc | depart,slight_right,arrive | ic        |
            | ie    | xi,xe,xe | depart,right,arrive        | ie        |

            | ko    | xk,xo,xo | depart,left,arrive         | ko        |
            | ka    | xk,xa,xa | depart,slight_left,arrive  | ka        |
            | kc    | xk,xc,xc | depart,straight,arrive     | kc        |
            | ke    | xk,xe,xe | depart,slight_right,arrive | ke        |
            | kg    | xk,xg,xg | depart,right,arrive        | kg        |

            | ma    | xm,xa,xa | depart,left,arrive         | ma        |
            | mc    | xm,xc,xc | depart,slight_left,arrive  | mc        |
            | me    | xm,xe,xe | depart,straight,arrive     | me        |
            | mg    | xm,xg,xg | depart,slight_right,arrive | mg        |
            | mi    | xm,xi,xi | depart,right,arrive        | mi        |

            | oc    | xo,xc,xc | depart,left,arrive         | oc        |
            | oe    | xo,xe,xe | depart,slight_left,arrive  | oe        |
            | og    | xo,xg,xg | depart,straight,arrive     | og        |
            | oi    | xo,xi,xi | depart,slight_right,arrive | oi        |
            | ok    | xo,xk,xk | depart,right,arrive        | ok        |

            | ae    | xa,xe,xe | depart,left,arrive         | ae        |
            | ag    | xa,xg,xg | depart,slight_left,arrive  | ag        |
            | ai    | xa,xi,xi | depart,straight,arrive     | ai        |
            | ak    | xa,xk,xk | depart,slight_right,arrive | ak        |
            | am    | xa,xm,xm | depart,right,arrive        | am        |

            | cg    | xc,xg,xg | depart,left,arrive         | cg        |
            | ci    | xc,xi,xi | depart,slight_left,arrive  | ci        |
            | ck    | xc,xk,xk | depart,straight,arrive     | ck        |
            | cm    | xc,xm,xm | depart,slight_right,arrive | cm        |
            | co    | xc,xo,xo | depart,right,arrive        | co        |

            | ei    | xe,xi,xi | depart,left,arrive         | ei        |
            | ek    | xe,xk,xk | depart,slight_left,arrive  | ek        |
            | em    | xe,xm,xm | depart,straight,arrive     | em        |
            | eo    | xe,xo,xo | depart,slight_right,arrive | eo        |
            | ea    | xe,xa,xa | depart,right,arrive        | ea        |

            | gk    | xg,xk,xk | depart,left,arrive         | gk        |
            | gm    | xg,xm,xm | depart,slight_left,arrive  | gm        |
            | go    | xg,xo,xo | depart,straight,arrive     | go        |
            | ga    | xg,xa,xa | depart,slight_right,arrive | ga        |
            | gc    | xg,xc,xc | depart,right,arrive        | gc        |

    Scenario: Turn directions
        Given the query options
            | instructions | true |
        Given the node map
            | o | p | a | b | c |
            | n |   |   |   | d |
            | m |   | x |   | e |
            | l |   |   |   | f |
            | k | j | i | h | g |

        And the ways
            | nodes |
            | xa    |
            | xc    |
            | xe    |
            | xg    |
            | xi    |
            | xk    |
            | xm    |
            | xo    |

        When I match I should get
            | trace | route    | turns                      | matchings | duration |
            | im    | xi,xm,xm | depart,left,arrive         | im        | 80       |
            | io    | xi,xo,xo | depart,slight_left,arrive  | io        | 88       |
            | ia    | xi,xa,xa | depart,straight,arrive     | ia        | 80       |
            | ic    | xi,xc,xc | depart,slight_right,arrive | ic        | 88       |
            | ie    | xi,xe,xe | depart,right,arrive        | ie        | 60       |
