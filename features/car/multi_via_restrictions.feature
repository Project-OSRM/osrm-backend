@routing @car @restrictions

Feature: Car - Multiple Via Turn restrictions

    Background: Use car routing
        Given the profile "car"
        Given a grid size of 200 meters


    @restriction-way @no_turning @overlap
    Scenario: Car - Node restriction inside multiple via restriction
        Given the node map
            """
              1   2   3          4         5
            a---b---c---d---e---------f---------g
                            |         |         |
                            |7        |8        |9
                            |         |         |
                        x---h---------i---------j
            """

        And the ways
            | nodes | oneway | name    |
            | ab    | yes    | forward |
            | bc    | yes    | forward |
            | cd    | yes    | forward |
            | de    | yes    | forward |
            | ef    | yes    | forward |
            | fg    | yes    | forward |
            | eh    | yes    | first   |
            | fi    | yes    | second  |
            | gj    | yes    | third   |
            | ih    | yes    | back    |
            | ji    | yes    | back    |
            | hx    | yes    | back    |

        And the relations
            | type        | way:from | way:via     | way:to | restriction   |
            | restriction | ab       | bc,cd,de,ef | fi     | no_right_turn |

        And the relations
            | type        | way:from | node:via | way:to | restriction   |
            | restriction | de       | e        | eh     | no_right_turn |

        When I route I should get
            | from | to | route                    |
            | 1    | x  | forward,third,back,back  |
            | 2    | x  | forward,second,back,back |
            | 3    | x  | forward,second,back,back |
            | 4    | x  | forward,second,back,back |
            | 5    | x  | forward,third,back,back  |
            | 7    | x  | first,back,back          |
            | 8    | x  | second,back,back         |
            | 9    | x  | third,back,back          |


    @restriction-way @no_turning @overlap @conditionals
    Scenario: Car - Conditional node restriction inside conditional multiple via restriction
        Given the origin -9.2972,10.3811
        # coordinate in Guinée, a country that observes GMT year round
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              1   2   3          4         5
            a---b---c---d---e---------f---------g
                            |         |         |
                            |7        |8        |9
                            |         |         |
                        x---h---------i---------j
            """

        And the ways
            | nodes | oneway | name    |
            | ab    | yes    | forward |
            | bc    | yes    | forward |
            | cd    | yes    | forward |
            | de    | yes    | forward |
            | ef    | yes    | forward |
            | fg    | yes    | forward |
            | eh    | yes    | first   |
            | fi    | yes    | second  |
            | gj    | yes    | third   |
            | ih    | yes    | back    |
            | ji    | yes    | back    |
            | hx    | yes    | back    |

        And the relations
            | type        | way:from | way:via     | way:to | restriction:conditional             |
            | restriction | ab       | bc,cd,de,ef | fi     | no_right_turn @ (Mo-Fr 07:00-10:30) |

        And the relations
            | type        | way:from | node:via | way:to | restriction:conditional               |
            | restriction | de       | e        | eh     | no_right_turn @ (Mo-Fr 07:00-10:30)   |
            | restriction | de       | e        | eh     | only_right_turn @ (Sa-Su 07:00-10:30) |

        When I route I should get
            | from | to | route                    |
            | 1    | x  | forward,third,back,back  |
            | 2    | x  | forward,second,back,back |
            | 3    | x  | forward,second,back,back |
            | 4    | x  | forward,second,back,back |
            | 5    | x  | forward,third,back,back  |
            | 7    | x  | first,back,back          |
            | 8    | x  | second,back,back         |
            | 9    | x  | third,back,back          |


    @restriction-way @no_turning @overlap
    Scenario: Car - Multiple via restriction inside multiple via restriction
        Given the node map
            """
              1   2   3          4         5
            a---b---c---d---e---------f---------g
                            |         |         |
                            |7        |8        |9
                            |         |         |
                        x---h---------i---------j
            """

        And the ways
            | nodes | oneway | name    |
            | ab    | yes    | forward |
            | bc    | yes    | forward |
            | cd    | yes    | forward |
            | de    | yes    | forward |
            | ef    | yes    | forward |
            | fg    | yes    | forward |
            | eh    | yes    | first   |
            | fi    | yes    | second  |
            | gj    | yes    | third   |
            | ih    | yes    | back    |
            | ji    | yes    | back    |
            | hx    | yes    | back    |

        And the relations
            | type        | way:from | way:via     | way:to | restriction   |
            | restriction | ab       | bc,cd,de,ef | fi     | no_right_turn |
            | restriction | bc       | cd,de       | eh     | no_right_turn |

        When I route I should get
            | from | to | route                    |
            | 1    | x  | forward,third,back,back  |
            | 2    | x  | forward,second,back,back |
            | 3    | x  | forward,first,back,back  |
            | 4    | x  | forward,second,back,back |
            | 5    | x  | forward,third,back,back  |
            | 7    | x  | first,back,back          |
            | 8    | x  | second,back,back         |
            | 9    | x  | third,back,back          |


    @restriction-way @no_turning @overlap @conditionals
    Scenario: Car - Conditional multiple via restriction inside conditional multiple via restriction

        Given a grid size of 200 meters
        Given the origin -9.2972,10.3811
        # coordinate in Guinée, a country that observes GMT year round
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map

            """
              1   2   3          4         5
            a---b---c---d---e---------f---------g
                            |         |         |
                            |7        |8        |9
                            |         |         |
                        x---h---------i---------j
            """

        And the ways
            | nodes | oneway | name    |
            | ab    | yes    | forward |
            | bc    | yes    | forward |
            | cd    | yes    | forward |
            | de    | yes    | forward |
            | ef    | yes    | forward |
            | fg    | yes    | forward |
            | eh    | yes    | first   |
            | fi    | yes    | second  |
            | gj    | yes    | third   |
            | ih    | yes    | back    |
            | ji    | yes    | back    |
            | hx    | yes    | back    |

        And the relations
            | type        | way:from | way:via     | way:to | restriction:conditional               |
            | restriction | ab       | bc,cd,de,ef | fi     | no_right_turn @ (Mo-Fr 07:00-10:30)   |
            | restriction | bc       | cd,de       | eh     | no_right_turn @ (Mo-Fr 07:00-10:30)   |
            | restriction | bc       | cd,de       | eh     | only_right_turn @ (Sa-Su 07:00-10:30) |

        When I route I should get
            | from | to | route                    |
            | 1    | x  | forward,third,back,back  |
            | 2    | x  | forward,second,back,back |
            | 3    | x  | forward,first,back,back  |
            | 4    | x  | forward,second,back,back |
            | 5    | x  | forward,third,back,back  |
            | 7    | x  | first,back,back          |
            | 8    | x  | second,back,back         |
            | 9    | x  | third,back,back          |


    @restriction-way @only_turning @overlap
    Scenario: Car - Overlapping multiple via restrictions
        Given the node map
            """
            a       f       j
            |       |       |
            b---d---e---i---k----m
            |       |       |
            c       g       l
            """

        And the ways
            | nodes | oneway | name    |
            | ab    | yes    | down    |
            | cb    | yes    | up      |
            | bd    | yes    | right   |
            | de    | yes    | right   |
            | ef    | yes    | up      |
            | eg    | yes    | down    |
            | ei    | yes    | right   |
            | ik    | yes    | right   |
            | kj    | yes    | up      |
            | kl    | yes    | down    |
            | km    | yes    | right   |

        And the relations
            | type        | way:from | way:via     | way:to | restriction      |
            | restriction | ab       | bd,de       | ei     | only_straight_on |
            | restriction | de       | ei,ik       | km     | only_straight_on |

        When I route I should get
            | from | to | route              |
            | a    | f  |                    |
            | a    | g  |                    |
            | a    | j  |                    |
            | a    | l  |                    |
            | a    | m  | down,right,right   |
            | c    | f  | up,right,up,up     |
            | c    | g  | up,right,down,down |
            | c    | j  |                    |
            | c    | l  |                    |
            | c    | m  | up,right,right     |
            | i    | j  | right,up,up        |
            | i    | l  | right,down,down    |
            | i    | m  | right,right        |


    @restriction-way @only_turning @overlap @conditionals
    Scenario: Car - Overlapping conditional multiple via restrictions
        Given a grid size of 200 meters
        Given the origin -9.2972,10.3811
        # coordinate in Guinée, a country that observes GMT year round
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
            a       f       j
            |       |       |
            b---d---e---i---k----m
            |       |       |
            c       g       l
            """

        And the ways
            | nodes | oneway | name   |
            | ab    | yes    | down   |
            | cb    | yes    | up     |
            | bd    | yes    | right  |
            | db    | yes    | left   |
            | de    | yes    | right  |
            | ed    | yes    | left   |
            | ef    | yes    | up     |
            | eg    | yes    | down   |
            | ei    | yes    | right  |
            | ie    | yes    | left   |
            | ik    | yes    | right  |
            | ki    | yes    | left   |
            | kj    | yes    | up     |
            | kl    | yes    | down   |
            | km    | no     | end    |

        And the relations
            | type        | way:from | way:via  | way:to | restriction:conditional                |
            | restriction | ab       | bd,de    | ei     | only_straight_on @ (Mo-Fr 07:00-10:30) |
            | restriction | ab       | bd,de    | ef     | only_left_turn   @ (Sa-Su 07:00-10:30) |
            | restriction | de       | ei,ik    | km     | only_straight_on @ (Mo-Fr 07:00-10:30) |

        When I route I should get
            | from | to | route                             |
            | a    | f  | down,right,end,end,left,up,up     |
            | a    | g  | down,right,end,end,left,down,down |
            | a    | j  | down,right,end,end,up,up          |
            | a    | l  | down,right,end,end,down,down      |
            | a    | m  | down,right,end,end                |
            | c    | f  | up,right,up,up                    |
            | c    | g  | up,right,down,down                |
            | c    | j  | up,right,end,end,up,up            |
            | c    | l  | up,right,end,end,down,down        |
            | c    | m  | up,right,end,end                  |
            | i    | j  | right,up,up                       |
            | i    | l  | right,down,down                   |
            | i    | m  | right,end,end                     |



    @restriction-way @only_turning @geometry
    Scenario: Car - Multiple via restriction with non-compressable via geometry
        Given the node map
            """
            a---b---c---d---e---f---g---h
                    |       |       |
                    i       j       k
            """

        And the ways
            | nodes | oneway | name   |
            | ab    | yes    | right  |
            | bcd   | yes    | right  |
            | defg  | yes    | right  |
            | ci    | yes    | down   |
            | ej    | yes    | down   |
            | gh    | yes    | end    |
            | gk    | yes    | down   |

        And the relations
            | type        | way:from | way:via    | way:to | restriction      |
            | restriction | ab       | bcd,defg   | gh     | only_straight_on |

        When I route I should get
            | from | to | route          |
            | a    | h  | right,end,end  |
            | a    | k  |                |

    @restriction-way @only_turning @geometry
    Scenario: Car - Multiple via restriction with non-compressable from/to nodes
        Given the node map
            """
            a---b---c---d---e---f---g---h---i---j---k---l
                    |       |       |       |       |
                    m       n       o       p       q
            """

        And the ways
            | nodes    | oneway | name   |
            | abcdefg  | yes    | right  |
            | ghi      | yes    | right  |
            | ijkl     | yes    | end    |
            | cm       | yes    | down   |
            | en       | yes    | down   |
            | go       | yes    | down   |
            | ip       | yes    | down   |
            | kq       | yes    | down   |

        And the relations
            | type        | way:from | way:via | way:to | restriction      |
            | restriction | abcdefg  | ghi     | ijkl   | only_straight_on |

        When I route I should get
            | from | to | route          |
            | a    | l  | right,end,end  |
            | a    | p  |                |

    @restriction-way @no_turning
    Scenario: Car - Long unrestricted route and short restricted route
        Given the node map
            """
            a------------------------------------b
            |                                    |
            c--d--e--f---------------------------
            """

        And the ways
            | nodes    |
            | ac       |
            | ab       |
            | bf       |
            | cd       |
            | de       |
            | ef       |

        And the relations
            | type        | way:from | way:via | way:to | restriction    |
            | restriction | ac       | cd      | de     | no_straight_on |

        When I route I should get
            | from | to | route     |
            | a    | f  | ab,bf,bf  |


    @restriction-way @overlap @no_turning
    Scenario: Car - Junction with multiple via u-turn restrictions
    # Example: https://www.openstreetmap.org/#map=19/52.07399/5.09724
        Given the node map
            """
                a   b
                |   |
            c---d---e---f
                |   |
            g---h---i---j
                |   |
                k   l
            """

        And the ways
            | nodes | oneway | name  |
            | ad    | yes    | down  |
            | eb    | yes    | up    |
            | fe    | yes    | left  |
            | ij    | yes    | right |
            | li    | yes    | up    |
            | hk    | yes    | down  |
            | gh    | yes    | right |
            | dc    | yes    | left  |
            | dh    | yes    | down  |
            | hi    | yes    | right |
            | ie    | yes    | up    |
            | ed    | yes    | left  |

        And the relations
            | type        | way:from | way:via  | way:to | restriction  |
            | restriction | ad       | dh,hi    | ie     | no_u_turn    |
            | restriction | li       | ie,ed    | dh     | no_u_turn    |

        When I route I should get
            | from | to | route                 |
            | a    | b  |                       |
            | a    | c  | down,left,left        |
            | a    | k  | down,down             |
            | a    | j  | down,right,right      |
            | f    | b  | left,up,up            |
            | f    | c  | left,left             |
            | f    | k  | left,down,down        |
            | f    | j  | left,down,right,right |
            | l    | b  | up,up                 |
            | l    | c  | up,left,left          |
            | l    | k  |                       |
            | l    | j  | up,right,right        |
            | g    | b  | right,up,up           |
            | g    | c  | right,up,left,left    |
            | g    | k  | right,down,down       |
            | g    | j  | right,right           |


    @restriction-way @overlap @no_turning
    Scenario: Car - Junction with multiple via u-turn restrictions, service roads
    # Example: https://www.openstreetmap.org/#map=19/48.38566/10.88068
        Given the node map
            """
                a     b
                |     |
            c---d--e--f---g
                |   _/|
                h__/  |
                |\ \  |
            i---j-k-l-m---n
                |     |
                o     p
            """

        And the ways
            | nodes | oneway | name  |
            | ad    | yes    | down  |
            | fb    | yes    | up    |
            | gf    | yes    | left  |
            | mn    | yes    | right |
            | pm    | yes    | up    |
            | jo    | yes    | down  |
            | ij    | yes    | right |
            | dc    | yes    | left  |
            | dh    | yes    | down  |
            | hj    | yes    | down  |
            | jkl   | yes    | right |
            | lm    | yes    | right |
            | mf    | yes    | up    |
            | fe    | yes    | left  |
            | ed    | yes    | left  |

        And the ways
            | nodes | oneway | name    | highway | access | psv |
            | kh    | yes    | service | service | no     | yes |
            | lh    | no     | service | service | no     | yes |
            | fh    | yes    | service | service | no     | yes |

        And the relations
            | type        | way:from | way:via    | way:to | restriction |
            | restriction | hj       | jkl,lm     | mf     | no_u_turn   |
            | restriction | lm       | mf         | fe     | no_u_turn   |
            | restriction | mf       | fe,ed      | dh     | no_u_turn   |
            | restriction | ed       | dh,hj      | jkl    | no_u_turn   |

        When I route I should get
            | from | to | route            |
            | a    | b  |                  |
            | a    | c  | down,left,left   |
            | a    | o  | down,down        |
            | a    | n  | down,right,right |
            | i    | b  | right,up,up      |
            | i    | c  |                  |
            | i    | o  | right,down,down  |
            | i    | n  | right,right      |
            | p    | b  | up,up            |
            | p    | c  | up,left,left     |
            | p    | o  |                  |
            | p    | n  | up,right,right   |
            | g    | b  | left,up,up       |
            | g    | c  | left,left        |
            | g    | o  | left,down,down   |
            | g    | n  |                  |


    @restriction-way @overlap @no_turning
    Scenario: Car - Junction with overlapping and duplicate multiple via restrictions
    # Example: https://www.openstreetmap.org/#map=19/34.66291/33.01711
        Given the profile file "car" initialized with
        """
        profile.properties.left_hand_driving = true
        """

        And the node map
            """
                a   b
                |   |
            c---d---e---f
                |   |
            g---h---i---j
                |   |
                k   l
            """

        And the nodes
            | node | highway         |
            | d    | traffic_signals |
            | e    | traffic_signals |
            | h    | traffic_signals |
            | i    | traffic_signals |

        And the ways
            | nodes | oneway | name  |
            | da    | yes    | up    |
            | be    | yes    | down  |
            | ef    | yes    | right |
            | ji    | yes    | left  |
            | il    | yes    | down  |
            | kh    | yes    | up    |
            | hg    | yes    | left  |
            | cd    | yes    | right |
            | hd    | yes    | up    |
            | ih    | yes    | left  |
            | ei    | yes    | down  |
            | de    | yes    | right |

        And the relations
            | type        | way:from | way:via    | way:to | restriction  |
            | restriction | be       | ei,ih      | hd     | no_u_turn    |
            | restriction | ji       | ih,hd      | de     | no_u_turn    |
            | restriction | kh       | hd,de      | ei     | no_u_turn    |
            | restriction | cd       | de,ei      | ih     | no_u_turn    |
            | restriction | hd       | de         | ei     | no_u_turn    |
            | restriction | de       | ei         | ih     | no_u_turn    |
            | restriction | ei       | ih         | hd     | no_u_turn    |
            | restriction | ei       | ih,hd      | de     | no_u_turn    |
            | restriction | ih       | hd         | de     | no_u_turn    |
            | restriction | ih       | hd,de      | ei     | no_u_turn    |

        When I route I should get
            | from | to | route             |
            | b    | a  |                   |
            | b    | g  | down,left,left    |
            | b    | l  | down,down         |
            | b    | f  | down,right,right  |
            | j    | a  | left,up,up        |
            | j    | g  | left,left         |
            | j    | l  | left,down,down    |
            | j    | f  |                   |
            | k    | a  | up,up             |
            | k    | g  | up,left,left      |
            | k    | l  |                   |
            | k    | f  | up,right,right    |
            | c    | a  | right,up,up       |
            | c    | g  |                   |
            | c    | l  | right,down,down   |
            | c    | f  | right,right       |


    @restriction-way @no_turning
    Scenario: Car - Junction with multiple via restriction to side road, traffic lights
    # Example: https://www.openstreetmap.org/#map=19/48.23662/16.42545
        Given the node map
            """
            e---d
                |
            f---c---g
                |
            h---b---i
                |
                a
            """

       And the nodes
            | node | highway         |
            | c    | traffic_signals |
            | b    | traffic_signals |

        And the ways
            | nodes | oneway | name  |
            | ab    | no     | up    |
            | bc    | no     | up    |
            | cd    | no     | up    |
            | de    | no     | left  |
            | hb    | yes    | right |
            | bi    | yes    | right |
            | gc    | yes    | left  |
            | cf    | yes    | left  |

        And the relations
            | type        | way:from | way:via    | way:to | restriction  |
            | restriction | ab       | bc,cd      | de     | no_left_turn |

        When I route I should get
            | from | to | route           |
            | a    | e  |                 |
            | a    | f  | up,left,left    |
            | a    | g  |                 |
            | a    | h  |                 |
            | a    | i  | up,right,right  |


    @restriction-way @overlap @no_turning
    Scenario: Car - Many overlapping multiple via restrictions, traffic signals
    # Example: https://www.openstreetmap.org/#map=19/48.76987/11.43410
        Given the node map
            """
             8                        5
            p______a_______n________o__x
           1|      |                |
            |       \              /
            r___q____b____        m
            |         \   \___  _/    7
            \          c      _l____k__w
             s          \   _/    _/
              \          _d/   __j
               \       _/  \ _/
               |   ___g     e_____i
            v__t__/       _/ \   4
             6          2/   3\
                       f       h
            """

       And the nodes
            | node | highway         |
            | n    | traffic_signals |
            | m    | traffic_signals |
            | q    | traffic_signals |

        And the ways
            | nodes | oneway |
            | on    | yes    |
            | na    | yes    |
            | ap    | yes    |
            | pr    | yes    |
            | rqb   | yes    |
            | bl    | yes    |
            | oml   | yes    |
            | ld    | yes    |
            | lk    | yes    |
            | ba    | yes    |
            | bcd   | no     |
            | de    | no     |
            | eh    | no     |
            | ei    | no     |
            | ejk   | yes    |
            | rst   | yes    |
            | dgt   | yes    |
            | fe    | yes    |
            | xo    | yes    |
            | tv    | yes    |
            | kw    | yes    |

        And the relations
            | type        | way:from | way:via    | way:to | restriction   |
            | restriction | pr       | rqb,bcd    | dgt    | no_right_turn |
            | restriction | rqb      | bcd,de     | ejk    | no_left_turn  |
            | restriction | rqb      | bcd        | dgt    | no_right_turn |
            | restriction | fe       | ed         | dgt    | no_u_turn     |
            | restriction | fe       | ed,dcb     | bl     | no_right_turn |
            | restriction | he       | ed,dcb     | bl     | no_right_turn |
            | restriction | oml      | ld,de      | ejk    | no_u_turn     |

        And the relations
            | type        | way:from | node:via   | way:to | restriction   |
            | restriction | ap       | p          | pr     | no_u_turn     |
            | restriction | rqb      | b          | ba     | no_left_turn  |
            | restriction | ld       | d          | dcb    | no_right_turn |
            | restriction | oml      | l          | lk     | no_left_turn  |
            | restriction | na       | a          | ab     | no_left_turn  |
            | restriction | dcb      | b          | bl     | no_right_turn |
            | restriction | dcb      | b          | bcd    | no_u_turn     |
            | restriction | bcd      | d          | dcb    | no_u_turn     |
            | restriction | bl       | l          | ld     | no_right_turn |

        # Additional relations to prevent u-turns on small roads polluting the results
        And the relations
            | type        | way:from | node:via   | way:to | restriction   |
            | restriction | eh       | h          | he     | no_u_turn     |
            | restriction | ei       | i          | ie     | no_u_turn     |

        When I route I should get
            | from | to | route               | locations   |
            | 1    | 6  | pr,rst,tv,tv        | _,r,t,_     |
            | 1    | 3  | pr,rqb,bcd,de,eh,eh | _,r,b,d,e,_ |
            | 1    | 4  | pr,rqb,bcd,de,ei,ei | _,r,b,d,e,_ |
            | 1    | 7  | pr,rqb,bl,lk,kw,kw  | _,r,b,l,k,_ |
            | 1    | 8  |                     |             |
            | 2    | 6  |                     |             |
            | 2    | 3  | fe,eh,eh            | _,e,_       |
            | 2    | 4  | fe,ei,ei            | _,e,_       |
            | 2    | 7  | fe,ejk,kw,kw        | _,e,k,_     |
            | 2    | 8  | fe,de,bcd,ba,ap,ap  | _,e,d,b,a,_ |
	    | 3    | 6  | eh,de,dgt,tv,tv     | _,e,d,t,_   |
	    | 3    | 4  | eh,ei,ei            | _,e,_       |
	    | 3    | 7  | eh,ejk,kw,kw        | _,e,k,_     |
	    | 3    | 8  | eh,de,bcd,ba,ap,ap  | _,e,d,b,a,_ |
	    | 4    | 6  | ei,de,dgt,tv,tv     | _,e,d,t,_   |
	    | 4    | 3  | ei,eh,eh            | _,e,_       |
	    | 4    | 7  | ei,ejk,kw,kw        | _,e,k,_     |
	    | 4    | 8  | ei,de,bcd,ba,ap,ap  | _,e,d,b,a,_ |
	    | 5    | 6  | xo,oml,ld,dgt,tv,tv | _,o,l,d,t,_ |
	    | 5    | 3  | xo,oml,ld,de,eh,eh  | _,o,l,d,e,_ |
	    | 5    | 4  | xo,oml,ld,de,ei,ei  | _,o,l,d,e,_ |
	    | 5    | 7  |                     |             |
	    | 5    | 8  | xo,on,na,ap,ap      | _,o,n,a,_   |



    @restriction-way @overlap @no_turning
    Scenario: Car - Multiple via restriction with start and end on same node
    # Example: https://www.openstreetmap.org/#map=19/52.41988/16.96088
        Given the node map
            """
            |--g---f---e
            a      |   |
            |--b---c---d

            """

       And the nodes
            | node | highway         |
            | b    | traffic_signals |

        And the ways
            | nodes | oneway | name  |
            | abc   | yes    | enter |
            | cd    | yes    | right |
            | de    | yes    | up    |
            | ef    | yes    | left  |
            | fga   | yes    | exit  |
            | fc    | yes    | down  |

        And the relations
            | type        | way:from | way:via    | way:to | restriction  |
            | restriction | abc      | cd,de,ef   | fga    | no_u_turn    |

        When I route I should get
            | from | to | route                                            | locations           |
            | a    | g  | enter,right,up,left,down,right,up,left,exit,exit | a,c,d,e,f,c,d,e,f,g |
            | b    | a  | enter,right,up,left,down,right,up,left,exit,exit | b,c,d,e,f,c,d,e,f,a |
        # This is a correct but not within the spirit of the restriction.
        # Does this indicate the restriction is not strong enough?


    @restriction-way @no_turning
    Scenario: Car - Multiple via restriction preventing bypassing main road
    # Example: https://www.openstreetmap.org/#map=19/48.72429/21.25912
        Given the node map
            """
            a--b--c--d--e--f
               \           |
                --g--h--i--j
                           |
                           k
            """

       And the nodes
            | node | highway         |
            | d    | traffic_signals |
            | e    | traffic_signals |

        And the ways
            | nodes | oneway | name  |
            | ab    | yes    | main  |
            | bc    | yes    | main  |
            | cd    | yes    | main  |
            | de    | yes    | main  |
            | ef    | yes    | main  |
            | bg    | yes    | side  |
            | gh    | yes    | side  |
            | hi    | yes    | side  |
            | ij    | yes    | side  |
            | fj    | yes    | turn  |
            | jk    | yes    | turn  |


        And the relations
            | type        | way:from | way:via     | way:to | restriction   |
            | restriction | ab       | bg,gh,hi,ij | jk     | no_right_turn |

        When I route I should get
            | from | to | route          |
            | a    | k  | main,turn,turn |


    @restriction-way @overlap @no_turning @only_turning
    Scenario: Car - Multiple via restriction with to,via,from sharing same node
    # Example: https://www.openstreetmap.org/relation/3972923
        Given the node map
            """
                e---d
                |   |
            a---b---c
                |
                f
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | deb   | yes    |
            | bf    | yes    |


        And the relations
            | type        | way:from | way:via     | way:to | restriction      |
            | restriction | ab       | bc,cd,deb   | bf     | no_u_turn        |

        And the relations
            | type        | way:from | node:via    | way:to | restriction      |
            | restriction | ab       | b           | bc     | only_straight_on |
            | restriction | deb      | b           | bc     | no_left_turn     |

        When I route I should get
            | from | to | route          |
            | a    | f  |                |
        # The last restriction is missing from OSM. Without it,
        # it produces the route: ab,bc,cd,deb,bc,cd,deb,bf,bf


    @restriction-way @except
    Scenario: Car - Multiple via restriction, exception applies
    # Example: https://www.openstreetmap.org/#map=19/50.04920/19.93251
        Given the node map
            """
            a---b---c---d--e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |


        And the relations
            | type        | way:from | way:via | way:to | restriction    | except   |
            | restriction | ab       | bc,cd   | de     | no_straight_on | motorcar |

        When I route I should get
            | from | to | route          |
            | a    | e  | ab,bc,cd,de,de |


    @restriction-way @except @no_turning
    Scenario: Car - Multiple via restriction, exception n/a
    # Example: https://www.openstreetmap.org/#map=19/50.04920/19.93251
        Given the node map
            """
            a---b---c---d--e
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |


        And the relations
            | type        | way:from | way:via | way:to | restriction    | except        |
            | restriction | ab       | bc,cd   | de     | no_straight_on | psv;emergency |

        When I route I should get
            | from | to | route |
            | a    | e  |       |


    @restriction-way @overlap @only_turning
    Scenario: Car - Multiple via restriction overlapping single via restriction
        Given the node map
            """
                    e
                    |
            a---b---c---d
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
            | from | to | route                  | turns                                                         | locations   |
            | a    | d  | abcd,ce,ce,abcd,abcd   | depart,turn left,continue uturn,turn left,arrive              | a,c,e,c,d   |
            | a    | e  | abcd,ce,ce             | depart,turn left,arrive                                       | a,c,e       |
            | a    | f  | abcd,hfb,hfb           | depart,turn right,arrive                                      | a,b,f       |
            | g    | e  | gf,hfb,abcd,abcd,ce,ce | depart,turn right,turn right,continue uturn,turn right,arrive | g,f,b,d,c,e |
            | g    | d  | gf,hfb,abcd,abcd       | depart,turn right,turn right,arrive                           | g,f,b,d     |
            | h    | e  | hfb,abcd,ce,ce         | depart,end of road right,turn left,arrive                     | h,b,c,e     |
            | h    | d  | hfb,abcd,abcd          | depart,end of road right,arrive                               | h,b,d       |


    @restriction-way
    Scenario: Ambiguous from/to ways
        Given the node map
            """
            a
            |
            b---d---e
            |       |
            c       f
            """

        And the ways
            | nodes |
            | abc   |
            | bd    |
            | de    |
            | ef    |

        And the relations
            | type        | way:from | way:via    | way:to | restriction    |
            | restriction | abc      | bd,de      | ef     | no_right_turn  |
            | restriction | ef       | de,bd      | abc    | no_right_turn  |

        When I route I should get
            | from | to | route            | locations |
	    | a    | f  | abc,bd,de,ef,ef  | a,b,d,e,f |
	    | f    | a  | ef,de,bd,abc,abc | f,e,d,b,a |
	    | c    | f  | abc,bd,de,ef,ef  | c,b,d,e,f |
	    | f    | c  | ef,de,bd,abc,abc | f,e,d,b,c |


    @restriction-way
    Scenario: Ambiguous via ways
        Given the node map
            """
            a
            |
            b---d---e---c
                    |
                    f
            """

        And the ways
            | nodes |
            | ab    |
            | bd    |
            | dec   |
            | ef    |

        And the relations
            | type        | way:from | way:via    | way:to | restriction    |
            | restriction | ab       | bd,dec     | ef     | no_right_turn  |
            | restriction | ef       | dec,bd     | ab     | no_right_turn  |

        When I route I should get
            | from | to | route           | locations |
	    | a    | f  | ab,bd,dec,ef,ef | a,b,d,e,f |
	    | f    | a  | ef,dec,bd,ab,ab | f,e,d,b,a |


    @restriction-way @invalid
    Scenario: Badly tagged restrictions
        Given the node map
            """
            a--b--c--d--e--f
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |

        And the relations
            | type        | way:from | way:via    | way:to | restriction    |
            | restriction | ab       | cd,de      | ef     | no_straight_on |
            | restriction | ab       | bc,de      | ef     | no_straight_on |
            | restriction | ab       | bc,cd      | ef     | no_straight_on |
            | restriction | ef       | de,cd      | bc     | no_straight_on |

        When I route I should get
            | from | to | route             | locations   |
	    | a    | f  | ab,bc,cd,de,ef,ef | a,b,c,d,e,f |


    @restriction-way
    Scenario: Snap source/target to via restriction way
        Given the node map
            """
            a-1-b-2-c-3-d
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        And the relations
            | type        | way:from | way:via | way:to  | restriction    |
            | restriction | ab       | bc      | cd      | no_straight_on |

        When I route I should get
            | from | to | route    |
            | 1    | 2  | ab,bc,bc |
            | 2    | 3  | bc,cd,cd |


    @restriction-way
    Scenario: Car - Snap source/target to multi-via restriction way
    # Example: https://www.openstreetmap.org/relation/11787041
        Given the node map
            """
            |--g---f---e
            a      |   1
            |--b---c---d

            """

       And the nodes
            | node | highway         |
            | b    | traffic_signals |

        And the ways
            | nodes | oneway | name  |
            | ab    | yes    | enter |
            | bc    | yes    | enter |
            | cd    | yes    | right |
            | de    | yes    | up    |
            | ef    | yes    | left  |
            | fc    | yes    | down  |
            | fg    | yes    | exit  |
            | ga    | yes    | exit  |

        And the relations
            | type        | way:from | way:via    | way:to | restriction  |
            | restriction | bc       | cd,de,ef   | fg     | no_u_turn    |

        When I route I should get
            | from | to | route              | locations           |
            | a    | 1  | enter,right,up,up  | a,c,d,_             |
            | 1    | a  | up,left,exit,exit  | _,e,f,a             |
