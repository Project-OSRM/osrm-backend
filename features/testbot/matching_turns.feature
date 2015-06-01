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
            | xa    |
            | xb    |
            | xc    |
            | xd    |
            | xe    |
            | xf    |
            | xg    |
            | xh    |
            | xi    |
            | xj    |
            | xk    |
            | xl    |
            | xm    |
            | xn    |
            | xo    |
            | xp    |

        When I match with turns I should get
            | trace | route | turns                         | matchings |
            | im    | xi,xm | head,left,destination         | im        |
            | io    | xi,xo | head,slight_left,destination  | io        |
            | ia    | xi,xa | head,straight,destination     | ia        |
            | ic    | xi,xc | head,slight_right,destination | ic        |
            | ie    | xi,xe | head,right,destination        | ie        |
            
            | ko    | xk,xo | head,left,destination         | ko        |
            | ka    | xk,xa | head,slight_left,destination  | ka        |
            | kc    | xk,xc | head,straight,destination     | kc        |
            | ke    | xk,xe | head,slight_right,destination | ke        |
            | kg    | xk,xg | head,right,destination        | kg        |

            | ma    | xm,xa | head,left,destination         | ma        |
            | mc    | xm,xc | head,slight_left,destination  | mc        |
            | me    | xm,xe | head,straight,destination     | me        |
            | mg    | xm,xg | head,slight_right,destination | mg        |
            | mi    | xm,xi | head,right,destination        | mi        |

            | oc    | xo,xc | head,left,destination         | oc        |
            | oe    | xo,xe | head,slight_left,destination  | oe        |
            | og    | xo,xg | head,straight,destination     | og        |
            | oi    | xo,xi | head,slight_right,destination | oi        |
            | ok    | xo,xk | head,right,destination        | ok        |

            | ae    | xa,xe | head,left,destination         | ae        |
            | ag    | xa,xg | head,slight_left,destination  | ag        |
            | ai    | xa,xi | head,straight,destination     | ai        |
            | ak    | xa,xk | head,slight_right,destination | ak        |
            | am    | xa,xm | head,right,destination        | am        |

            | cg    | xc,xg | head,left,destination         | cg        |
            | ci    | xc,xi | head,slight_left,destination  | ci        |
            | ck    | xc,xk | head,straight,destination     | ck        |
            | cm    | xc,xm | head,slight_right,destination | cm        |
            | co    | xc,xo | head,right,destination        | co        |

            | ei    | xe,xi | head,left,destination         | ei        |
            | ek    | xe,xk | head,slight_left,destination  | ek        |
            | em    | xe,xm | head,straight,destination     | em        |
            | eo    | xe,xo | head,slight_right,destination | eo        |
            | ea    | xe,xa | head,right,destination        | ea        |

            | gk    | xg,xk | head,left,destination         | gk        |
            | gm    | xg,xm | head,slight_left,destination  | gm        |
            | go    | xg,xo | head,straight,destination     | go        |
            | ga    | xg,xa | head,slight_right,destination | ga        |
            | gc    | xg,xc | head,right,destination        | gc        |
