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
            | trace | route | turns                      | matchings |
            | im    | xi,xm | depart,left,arrive         | im        |
            | io    | xi,xo | depart,slight_left,arrive  | io        |
            | ia    | xi,xa | depart,straight,arrive     | ia        |
            | ic    | xi,xc | depart,slight_right,arrive | ic        |
            | ie    | xi,xe | depart,right,arrive        | ie        |

            | ko    | xk,xo | depart,left,arrive         | ko        |
            | ka    | xk,xa | depart,slight_left,arrive  | ka        |
            | kc    | xk,xc | depart,straight,arrive     | kc        |
            | ke    | xk,xe | depart,slight_right,arrive | ke        |
            | kg    | xk,xg | depart,right,arrive        | kg        |

            | ma    | xm,xa | depart,left,arrive         | ma        |
            | mc    | xm,xc | depart,slight_left,arrive  | mc        |
            | me    | xm,xe | depart,straight,arrive     | me        |
            | mg    | xm,xg | depart,slight_right,arrive | mg        |
            | mi    | xm,xi | depart,right,arrive        | mi        |

            | oc    | xo,xc | depart,left,arrive         | oc        |
            | oe    | xo,xe | depart,slight_left,arrive  | oe        |
            | og    | xo,xg | depart,straight,arrive     | og        |
            | oi    | xo,xi | depart,slight_right,arrive | oi        |
            | ok    | xo,xk | depart,right,arrive        | ok        |

            | ae    | xa,xe | depart,left,arrive         | ae        |
            | ag    | xa,xg | depart,slight_left,arrive  | ag        |
            | ai    | xa,xi | depart,straight,arrive     | ai        |
            | ak    | xa,xk | depart,slight_right,arrive | ak        |
            | am    | xa,xm | depart,right,arrive        | am        |

            | cg    | xc,xg | depart,left,arrive         | cg        |
            | ci    | xc,xi | depart,slight_left,arrive  | ci        |
            | ck    | xc,xk | depart,straight,arrive     | ck        |
            | cm    | xc,xm | depart,slight_right,arrive | cm        |
            | co    | xc,xo | depart,right,arrive        | co        |

            | ei    | xe,xi | depart,left,arrive         | ei        |
            | ek    | xe,xk | depart,slight_left,arrive  | ek        |
            | em    | xe,xm | depart,straight,arrive     | em        |
            | eo    | xe,xo | depart,slight_right,arrive | eo        |
            | ea    | xe,xa | depart,right,arrive        | ea        |

            | gk    | xg,xk | depart,left,arrive         | gk        |
            | gm    | xg,xm | depart,slight_left,arrive  | gm        |
            | go    | xg,xo | depart,straight,arrive     | go        |
            | ga    | xg,xa | depart,slight_right,arrive | ga        |
            | gc    | xg,xc | depart,right,arrive        | gc        |

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
            | trace | route | turns                      | matchings | duration |
            | im    | xi,xm | depart,left,arrive         | im        | 80       |
            | io    | xi,xo | depart,slight_left,arrive  | io        | 88       |
            | ia    | xi,xa | depart,straight,arrive     | ia        | 80       |
            | ic    | xi,xc | depart,slight_right,arrive | ic        | 88       |
            | ie    | xi,xe | depart,right,arrive        | ie        | 60       |
