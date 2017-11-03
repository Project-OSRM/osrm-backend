@routing @car @restrictions
Feature: Car - Turn restrictions
# Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
# Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.

    Background: Use car routing
        Given the profile "car"
        Given a grid size of 200 meters

    @no_turning
    Scenario: Car - No left turn
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | cj       | dj     | j        | no_left_turn |

        When I route I should get
            | from | to | route    |
            | c    | d  |          |
            | c    | a  | cj,aj,aj |
            | c    | b  | cj,bj,bj |

    @no_turning
    Scenario: Car - No straight on
        Given the node map
            """
            a b j d e
            v       z
              w x y
            """

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | bj    | no     |
            | jd    | no     |
            | de    | no     |
            | av    | yes    |
            | vw    | yes    |
            | wx    | yes    |
            | xy    | yes    |
            | yz    | yes    |
            | ze    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction    |
            | restriction | bj       | jd     | j        | no_straight_on |

        When I route I should get
            | from | to | route                |
            | a    | e  | av,vw,wx,xy,yz,ze,ze |

    @no_turning
    Scenario: Car - No right turn
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | cj       | bj     | j        | no_right_turn |

        When I route I should get
            | from | to | route    |
            | c    | d  | cj,dj,dj |
            | c    | a  | cj,aj,aj |
            | c    | b  |          |

    @no_turning
    Scenario: Car - No u-turn
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction |
            | restriction | cj       | dj     | j        | no_u_turn   |

        When I route I should get
            | from | to | route    |
            | c    | d  |          |
            | c    | a  | cj,aj,aj |
            | c    | b  | cj,bj,bj |

    @no_turning
    Scenario: Car - Handle any no_* relation
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | cj       | dj     | j        | no_weird_zigzags |

        When I route I should get
            | from | to | route    |
            | c    | d  |          |
            | c    | a  | cj,aj,aj |
            | c    | b  | cj,bj,bj |

    @no_turning
    Scenario: Car - Ignore no_*_on_red relations
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction          |
            | restriction | cj       | dj     | j        | no_turn_on_red       |
            | restriction | cj       | bj     | j        | no_right_turn_on_red |

        When I route I should get
            | from | to | route    |
            | c    | d  | cj,dj,dj |
            | c    | a  | cj,aj,aj |
            | c    | b  | cj,bj,bj |

    @only_turning
    Scenario: Car - Only left turn
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction    |
            | restriction | cj       | dj     | j        | only_left_turn |

        When I route I should get
            | from | to | route    |
            | c    | a  |          |
            | c    | b  |          |
            | c    | d  | cj,dj,dj |

    Scenario: Car - Only right turn, invalid
        Given the node map
            """
              a
            d j b r
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |
            | rb    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | cj       | br     | j        | only_right_on |

        When I route I should get
            | from | to | route       |
            | c    | r  | cj,bj,rb,rb |

    @only_turning
    Scenario: Car - Only right turn
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction     |
            | restriction | cj       | bj     | j        | only_right_turn |

        When I route I should get
            | from | to | route    |
            | c    | d  |          |
            | c    | a  |          |
            | c    | b  | cj,bj,bj |

    @only_turning
    Scenario: Car - Only straight on
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | cj       | aj     | j        | only_straight_on |

        When I route I should get
            | from | to | route    |
            | c    | d  |          |
            | c    | a  | cj,aj,aj |
            | c    | b  |          |

    @no_turning
    Scenario: Car - Handle any only_* restriction
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction        |
            | restriction | cj       | aj     | j        | only_weird_zigzags |

        When I route I should get
            | from | to | route    |
            | c    | d  |          |
            | c    | a  | cj,aj,aj |
            | c    | b  |          |

    @specific
    Scenario: Car - :hgv-qualified on a standard turn restriction
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction:hgv |
            | restriction | cj       | aj     | j        | no_straight_on  |

        When I route I should get
            | from | to | route    |
            | c    | d  | cj,dj,dj |
            | c    | a  | cj,aj,aj |
            | c    | b  | cj,bj,bj |

    @specific
    Scenario: Car - :motorcar-qualified on a standard turn restriction
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction:motorcar |
            | restriction | cj       | aj     | j        | no_straight_on       |

        When I route I should get
            | from | to | route    |
            | c    | d  | cj,dj,dj |
            | c    | a  |          |
            | c    | b  | cj,bj,bj |

    @except
    Scenario: Car - Except tag and on no_ restrictions
        Given the node map
            """
            b x c
            a j d
              s
            """

        And the ways
            | nodes | oneway |
            | sj    | no     |
            | xj    | -1     |
            | aj    | -1     |
            | bj    | no     |
            | cj    | no     |
            | dj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   | except   |
            | restriction | sj       | aj     | j        | no_left_turn  | motorcar |
            | restriction | sj       | bj     | j        | no_left_turn  |          |
            | restriction | sj       | cj     | j        | no_right_turn |          |
            | restriction | sj       | dj     | j        | no_right_turn | motorcar |

        When I route I should get
            | from | to | route    |
            | s    | a  | sj,aj,aj |
            | s    | b  |          |
            | s    | c  |          |
            | s    | d  | sj,dj,dj |

    @except
    Scenario: Car - Except tag and on only_ restrictions
        Given the node map
            """
            a   b
              j
              s
            """

        And the ways
            | nodes | oneway |
            | sj    | yes    |
            | aj    | no     |
            | bj    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      | except   |
            | restriction | sj       | aj     | j        | only_straight_on | motorcar |

        When I route I should get
            | from | to | route    |
            | s    | a  | sj,aj,aj |
            | s    | b  | sj,bj,bj |

    @except
    Scenario: Car - Several only_ restrictions at the same segment
        Given the node map
            """
                    y
            i j f b x a e g h

                  c   d
            """

        And the ways
            | nodes | oneway |
            | fb    | no     |
            | bx    | -1     |
            | xa    | no     |
            | ae    | no     |
            | cb    | no     |
            | dc    | -1     |
            | da    | no     |
            | fj    | no     |
            | jf    | no     |
            | ge    | no     |
            | hg    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | ae       | xa     | a        | only_straight_on |
            | restriction | xb       | fb     | b        | only_straight_on |
            | restriction | cb       | bx     | b        | only_right_turn  |
            | restriction | da       | ae     | a        | only_right_turn  |

        When I route I should get
            | from | to | route                               |
            | e    | f  | ae,xa,bx,fb,fb                      |
            | c    | f  | dc,da,ae,ge,hg,hg,ge,ae,xa,bx,fb,fb |
            | d    | f  | da,ae,ge,hg,hg,ge,ae,xa,bx,fb,fb    |

    @except
    Scenario: Car - two only_ restrictions share same to-way
        Given the node map
            """
                e       f
                    a

                c   x   d
                    y

                    b
            """

        And the ways
            | nodes | oneway |
            | ef    | no     |
            | ce    | no     |
            | fd    | no     |
            | ca    | no     |
            | ad    | no     |
            | ax    | no     |
            | xy    | no     |
            | yb    | no     |
            | cb    | no     |
            | db    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | ax       | xy     | x        | only_straight_on |
            | restriction | by       | xy     | y        | only_straight_on |

        When I route I should get
            | from | to | route       |
            | a    | b  | ax,xy,yb,yb |
            | b    | a  | yb,xy,ax,ax |

    @except
    Scenario: Car - two only_ restrictions share same from-way
        Given the node map
            """
                e       f
                    a

                c   x   d
                    y

                    b
            """

        And the ways
            | nodes | oneway |
            | ef    | no     |
            | ce    | no     |
            | fd    | no     |
            | ca    | no     |
            | ad    | no     |
            | ax    | no     |
            | xy    | no     |
            | yb    | no     |
            | cb    | no     |
            | db    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | xy       | xa     | x        | only_straight_on |
            | restriction | xy       | yb     | y        | only_straight_on |

        When I route I should get
            | from | to | route       |
            | a    | b  | ax,xy,yb,yb |
            | b    | a  | yb,xy,ax,ax |

    @specific
    Scenario: Car - Ignore unrecognized restriction
        Given the node map
            """
              a
            d j b
              c
            """

        And the ways
            | nodes | oneway |
            | cj    | yes    |
            | aj    | -1     |
            | dj    | -1     |
            | bj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | cj       | dj     | j        | yield        |

        When I route I should get
            | from | to | route    |
            | c    | d  | cj,dj,dj |
            | c    | a  | cj,aj,aj |
            | c    | b  | cj,bj,bj |

    @restriction @compression
    Scenario: Restriction On Compressed Geometry
        Given the node map
            """
                        i
                        |
                    f - e
                    |   |
            a - b - c - d
                    |
                    g
                    |
                    h
            """

        And the ways
            | nodes |
            | abc   |
            | cde   |
            | efc   |
            | cgh   |
            | ei    |

        And the relations
            | type        | way:from | node:via | way:to | restriction   |
            | restriction | abc      | c        | cgh    | no_right_turn |

        When I route I should get
            | from | to | route               |
            | a    | h  | abc,cde,efc,cgh,cgh |

    @restriction-way
    Scenario: Car - prohibit turn
        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            a   d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | be      | de     | no_right_turn |

        When I route I should get
            | from | to | route             | turns                                                               | locations   |
            | a    | d  | ab,be,ef,ef,de,de | depart,turn right,turn left,continue uturn,new name straight,arrive | a,b,e,f,e,d |
            | a    | f  | ab,be,ef,ef       | depart,turn right,turn left,arrive                                  | a,b,e,f     |
            | c    | d  | bc,be,de,de       | depart,turn left,turn right,arrive                                  | c,b,e,d     |
            | c    | f  | bc,be,ef,ef       | depart,turn left,turn left,arrive                                   | c,b,e,f     |

    @restriction-way @overlap
    Scenario: Car - prohibit turn
        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            |   d
            |
            a
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | be      | de     | no_right_turn |
            | restriction | bc       | be      | ef     | no_left_turn  |

        When I route I should get
            | from | to | route             |
            | a    | d  | ab,be,ef,ef,de,de |
            | a    | f  | ab,be,ef,ef       |
            | c    | d  | bc,be,de,de       |
            | c    | f  | bc,be,de,de,ef,ef |

    @restriction-way @overlap
    Scenario: Two times same way
        Given the node map
            """
                h   g
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
                |   |
            a - b - c - - - - - - - - - - - - - - - - - - - f
                |   | \                                     /
                i - d - e - - - - - - - - - - - - - - - - -
            """
        # The long distances here are required to make other turns undesriable in comparison to the restricted turns.
        # Otherwise they might just be picked without the actual turns being restricted

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | bc    | no     |
            | cd    | yes    |
            | ce    | yes    |
            | cf    | yes    |
            | cg    | yes    |
            | bh    | no     |
            | fedib | yes    |

       And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | bc      | ce     | no_right_turn |
            | restriction | ab       | bc      | cd     | no_right_turn |

       When I route I should get
            | from | to | route                |
            | a    | i  | ab,bc,cf,fedib,fedib |


    @restriction-way @overlap
    Scenario: Car - prohibit turn
        Given the node map
            """
            a   j
            |   |
            b---i
            |   |
            c---h
            |   |
            d---g
            |   |
            e   f
            """

        And the ways
            | nodes | name   | oneway |
            | ab    | left   | yes    |
            | bc    | left   | yes    |
            | cd    | left   | yes    |
            | de    | left   | yes    |
            | fg    | right  | yes    |
            | gh    | right  | yes    |
            | hi    | right  | yes    |
            | ij    | right  | yes    |
            | dg    | first  | no     |
            | ch    | second | no     |
            | bi    | third  | no     |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | bi      | ij     | no_u_turn     |
            | restriction | bc       | ch      | hi     | no_u_turn     |
            | restriction | fg       | dg      | de     | no_u_turn     |
            | restriction | gh       | ch      | cd     | no_u_turn     |

        When I route I should get
            | from | to | route                  |
            | a    | j  | left,first,right,right |
            | f    | e  | right,third,left,left  |

    @restriction-way
    Scenario: Car - allow only turn
        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            a   d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction  |
            | restriction | ab       | be      | ef     | only_left_on |

        When I route I should get
            | from | to | route       | turns | locations |
            | a    | d  | ab,be,ef,ef,de,de | depart,turn right,turn left,continue uturn,new name straight,arrive | a,b,e,f,e,d |
            | a    | f  | ab,be,ef,ef       | depart,turn right,turn left,arrive                                  | a,b,e,f     |
            | c    | d  | bc,be,de,de       | depart,turn left,turn right,arrive                                  | c,b,e,d     |
            | c    | f  | bc,be,ef,ef       | depart,turn left,turn left,arrive                                   | c,b,e,f     |

    @restriction-way
    Scenario: Car - allow only turn
        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            a   d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | be    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | be      | ed     | only_right_on |

        When I route I should get
            | from | to | route       |
            | a    | d  | ab,be,de,de |

    @restriction-way
    Scenario: Multi Way restriction
        Given the node map
            """
                  k   j
                  |   |
            h - - g - f - - e
                  |   |
                  |   |
            a - - b - c - - d
                  |   |
                  l   i
            """

        And the ways
            | nodes | name  | oneway |
            | ab    | horiz | yes    |
            | bc    | horiz | yes    |
            | cd    | horiz | yes    |
            | ef    | horiz | yes    |
            | fg    | horiz | yes    |
            | gh    | horiz | yes    |
            | ic    | vert  | yes    |
            | cf    | vert  | yes    |
            | fj    | vert  | yes    |
            | kg    | vert  | yes    |
            | gb    | vert  | yes    |
            | bl    | vert  | yes    |

        And the relations
            | type        | way:from | way:via  | way:to | restriction |
            | restriction | ab       | bc,cf,fg | gh     | no_u_turn   |

        When I route I should get
            | from | to | route                  |
            | a    | h  | horiz,vert,horiz,horiz |

    @restriction-way
    Scenario: Multi-Way overlapping single-way
        Given the node map
            """
                    e
                    |
            a - b - c - d
                |
                f - g
                |
                h
            """

        And the ways
            | nodes | name |
            | ab    | abcd |
            | bc    | abcd |
            | cd    | abcd |
            | hf    | hfb  |
            | fb    | hfb  |
            | gf    | gf   |
            | ce    | ce   |

        And the relations
            | type        | way:from | way:via | way:to | restriction    |
            | restriction | ab       | bc      | ce     | only_left_turn |
            | restriction | gf       | fb,bc   | cd     | only_u_turn    |

        When I route I should get
            | from | to | route                | turns                                            | locations |
            | a    | d  | abcd,ce,ce,abcd,abcd | depart,turn left,continue uturn,turn left,arrive | a,c,e,c,d |
            | a    | e  | abcd,ce,ce           | depart,turn left,arrive                          | a,c,e     |
            | a    | f  | abcd,hfb,hfb         | depart,turn right,arrive                         | a,b,f     |
            | g    | e  | gf,hfb,abcd,ce,ce    | depart,turn right,turn right,turn left,arrive    | g,f,b,c,e |
            | g    | d  | gf,hfb,abcd,abcd     | depart,turn right,turn right,arrive              | g,f,b,d   |
            | h    | e  | hfb,abcd,ce,ce       | depart,end of road right,turn left,arrive        | h,b,c,e   |
            | h    | d  | hfb,abcd,abcd        | depart,end of road right,arrive                  | h,b,d     |


    @restriction-way
    Scenario: Car - prohibit turn, traffic lights
        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            a   d
            |   |
            g   i
            |   |
            h   j
            """

        And the ways
            | nodes | name |
            | hgab  | ab   |
            | bc    | bc   |
            | be    | be   |
            | jide  | de   |
            | ef    | ef   |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | hgab     | be      | jide   | no_right_turn |

        And the nodes
            | node | highway         |
            | g    | traffic_signals |
            | i    | traffic_signals |


        When I route I should get
            | from | to | route             | turns                                                               | locations   |
            | a    | d  | ab,be,ef,ef,de,de | depart,turn right,turn left,continue uturn,new name straight,arrive | a,b,e,f,e,d |
            | a    | f  | ab,be,ef,ef       | depart,turn right,turn left,arrive                                  | a,b,e,f     |
            | c    | d  | bc,be,de,de       | depart,turn left,turn right,arrive                                  | c,b,e,d     |
            | c    | f  | bc,be,ef,ef       | depart,turn left,turn left,arrive                                   | c,b,e,f     |


      @restriction-way @overlap @geometry
      Scenario: Geometry
        Given the node map
            """
            c
            |
            |   f
            |   |
            b-g-e
            |   |
            |   d
            |
            a
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bge   |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | bge     | de     | no_right_turn |
            | restriction | bc       | bge     | ef     | no_left_turn  |

        When I route I should get
            | from | to | route              |
            | a    | d  | ab,bge,ef,ef,de,de |
            | a    | f  | ab,bge,ef,ef       |
            | c    | d  | bc,bge,de,de       |
            | c    | f  | bc,bge,de,de,ef,ef |

      @restriction-way @overlap @geometry @traffic-signals
      Scenario: Geometry
        Given the node map
            """
            c
            |
            |   f
            |   |
            b-g-e
            |   |
            |   d
            |
            a
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bge   |
            | de    |
            | ef    |

        And the nodes
            | node | highway         |
            | g    | traffic_signals |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | bge     | de     | no_right_turn |
            | restriction | bc       | bge     | ef     | no_left_turn  |

        # this case is currently not handling the via-way restrictions and we need support for looking across traffic signals.
        # It is mainly included to show limitations and to prove that we don't crash hard here
        When I route I should get
            | from | to | route              |
            | a    | d  | ab,bge,ef,ef,de,de |
            | a    | f  | ab,bge,ef,ef       |
            | c    | d  | bc,bge,de,de       |
            | c    | f  | bc,bge,de,de,ef,ef |

      # don't crash hard on invalid restrictions
      @restriction-way @invalid
      Scenario: Geometry
        Given the node map
            """
            c
            |
            |   f
            |   |
            b---e
            |   |
            |   d
            |
            a
            """

        And the ways
            | nodes | oneway |
            | ab    |        |
            | bc    |        |
            | be    | yes    |
            | de    |        |
            | ef    |        |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | de       | be      | ab     | no_left_turn  |

        When I route I should get
            | from | to | route       |
            | a    | f  | ab,be,ef,ef |


    @restriction @restriction-way @overlap @geometry
    Scenario: Duplicated restriction
        Given the node map
            """
            c
            |
            |   f
            |   |
            b-g-e
            |   |
            |   d
            |
            a
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bge   |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | bge      | ef     | e        | no_left_turn |

        And the relations
            | type        | way:from | way:via | way:to | restriction   |
            | restriction | ab       | bge     | de     | no_right_turn |
            | restriction | bc       | bge     | ef     | no_left_turn  |

        When I route I should get
            | from | to | route              |
            | a    | d  | ab,bc,bc,bge,de,de |


    Scenario: Ambiguous ways
        Given the node map
            """
            x---a----b-----c---z
                     |
                     d
            """

        And the ways
            | nodes |
            | abc   |
            | bd    |
            | xa    |
            | cz    |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | bd       | abc    | b        | no_left_turn |

        When I route I should get
            | from | to | route        |
            | d    | x  | bd,abc,xa,xa |
            | d    | z  | bd,abc,cz,cz |
