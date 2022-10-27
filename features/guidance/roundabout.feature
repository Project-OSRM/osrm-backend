@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Only Enter
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route          | turns                                   |
           | a,c       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | a,e       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | a,g       | ab,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,e       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,g       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | d,b       | cd,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,g       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,b       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | f,c       | ef,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,b       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,c       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |
           | h,e       | gh,bcegb,bcegb | depart,roundabout-exit-undefined,arrive |

    Scenario: Roundabout With Service
        Given the node map
            """
                a h
                bg
            d c
                e
                f
            """

        And the ways
            | nodes  | junction   | highway |
            | ab     |            | primary |
            | cd     |            | primary |
            | ef     |            | service |
            | gh     |            | primary |
            | bcegb  | roundabout | primary |

        When I route I should get
            | waypoints | route       | turns                                                    |
            | a,d       | ab,cd,cd,cd | depart,roundabout-exit-1,exit roundabout right,arrive    |
            | a,h       | ab,gh,gh,gh | depart,roundabout-exit-2,exit roundabout straight,arrive |
            | a,f       | ab,ef,ef,ef | depart,roundabout-exit-2,exit roundabout right,arrive    |

    #2927
    Scenario: Only Roundabout
        Given the node map
            """
              a
            b   d
              c
            """

       And the ways
            | nodes  | junction   |
            | abcda  | roundabout |

       When I route I should get
           | waypoints | route       | turns         |
           | a,c       | abcda,abcda | depart,arrive |

    Scenario: Only Exit
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route       | turns                               |
           | b,d       | bcegb,cd,cd | depart,exit roundabout right,arrive |
           | b,f       | bcegb,ef,ef | depart,exit roundabout right,arrive |
           | b,h       | bcegb,gh,gh | depart,exit roundabout right,arrive |
           | c,f       | bcegb,ef,ef | depart,exit roundabout right,arrive |
           | c,h       | bcegb,gh,gh | depart,exit roundabout right,arrive |
           | c,a       | bcegb,ab,ab | depart,exit roundabout right,arrive |
           | e,h       | bcegb,gh,gh | depart,exit roundabout right,arrive |
           | e,a       | bcegb,ab,ab | depart,exit roundabout right,arrive |
           | e,d       | bcegb,cd,cd | depart,exit roundabout right,arrive |
           | g,a       | bcegb,ab,ab | depart,exit roundabout right,arrive |
           | g,d       | bcegb,cd,cd | depart,exit roundabout right,arrive |
           | g,f       | bcegb,ef,ef | depart,exit roundabout right,arrive |
      #phantom node snapping can result in a full round-trip here, therefore we cannot test b->a and the other direct exits

    Scenario: Drive Around
        Given the node map
            """
                a
                b
            d c   g h
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route       | turns         |
           | b,c       | bcegb,bcegb | depart,arrive |
           | b,e       | bcegb,bcegb | depart,arrive |
           | b,g       | bcegb,bcegb | depart,arrive |
           | c,e       | bcegb,bcegb | depart,arrive |
           | c,g       | bcegb,bcegb | depart,arrive |
           | c,b       | bcegb,bcegb | depart,arrive |
           | e,g       | bcegb,bcegb | depart,arrive |
           | e,b       | bcegb,bcegb | depart,arrive |
           | e,c       | bcegb,bcegb | depart,arrive |
           | g,b       | bcegb,bcegb | depart,arrive |
           | g,c       | bcegb,bcegb | depart,arrive |
           | g,e       | bcegb,bcegb | depart,arrive |

     Scenario: Mixed Entry and Exit
        Given the node map
           """
             c   a
           j   b   f
             k   e
           l   h   d
             g   i
           """

        And the ways
           | nodes | junction   | oneway |
           | abc   |            | yes    |
           | def   |            | yes    |
           | ghi   |            | yes    |
           | jkl   |            | yes    |
           | bkheb | roundabout | yes    |

        When I route I should get
           | waypoints | route           | turns                                                    |
           | a,c       | abc,abc,abc     | depart,exit roundabout right,arrive                      |
           | a,l       | abc,jkl,jkl,jkl | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | a,i       | abc,ghi,ghi,ghi | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | a,f       | abc,def,def,def | depart,roundabout-exit-4,exit roundabout straight,arrive |
           | d,f       | def,def,def     | depart,exit roundabout right,arrive                      |
           | d,c       | def,abc,abc,abc | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | d,l       | def,jkl,jkl,jkl | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | d,i       | def,ghi,ghi,ghi | depart,roundabout-exit-4,exit roundabout straight,arrive |
           | g,i       | ghi,ghi,ghi     | depart,exit roundabout right,arrive                      |
           | g,f       | ghi,def,def,def | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | g,c       | ghi,abc,abc,abc | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | g,l       | ghi,jkl,jkl,jkl | depart,roundabout-exit-4,exit roundabout straight,arrive |
           | j,l       | jkl,jkl,jkl     | depart,exit roundabout right,arrive                      |
           | j,i       | jkl,ghi,ghi,ghi | depart,roundabout-exit-2,exit roundabout straight,arrive |
           | j,f       | jkl,def,def,def | depart,roundabout-exit-3,exit roundabout straight,arrive |
           | j,c       | jkl,abc,abc,abc | depart,roundabout-exit-4,exit roundabout straight,arrive |

    Scenario: Mixed Entry and Exit - segregated roads
        Given the node map
           """
               a   c

           l     b     d
               k   e
           j     h     f

               i   g
           """

        And the ways
           | nodes | junction   | oneway |
           | abc   |            | yes    |
           | def   |            | yes    |
           | ghi   |            | yes    |
           | jkl   |            | yes    |
           | bkheb | roundabout | yes    |

        When I route I should get
           | waypoints | route           | turns                                                 |
           | a,c       | abc,abc,abc,abc | depart,roundabout-exit-4,exit roundabout right,arrive |
           | a,l       | abc,jkl,jkl,jkl | depart,roundabout-exit-1,exit roundabout right,arrive |
           | a,i       | abc,ghi,ghi,ghi | depart,roundabout-exit-2,exit roundabout right,arrive |
           | a,f       | abc,def,def,def | depart,roundabout-exit-3,exit roundabout right,arrive |
           | d,f       | def,def,def,def | depart,roundabout-exit-4,exit roundabout right,arrive |
           | d,c       | def,abc,abc,abc | depart,roundabout-exit-1,exit roundabout right,arrive |
           | d,l       | def,jkl,jkl,jkl | depart,roundabout-exit-2,exit roundabout right,arrive |
           | d,i       | def,ghi,ghi,ghi | depart,roundabout-exit-3,exit roundabout right,arrive |
           | g,i       | ghi,ghi,ghi,ghi | depart,roundabout-exit-4,exit roundabout right,arrive |
           | g,f       | ghi,def,def,def | depart,roundabout-exit-1,exit roundabout right,arrive |
           | g,c       | ghi,abc,abc,abc | depart,roundabout-exit-2,exit roundabout right,arrive |
           | g,l       | ghi,jkl,jkl,jkl | depart,roundabout-exit-3,exit roundabout right,arrive |
           | j,l       | jkl,jkl,jkl,jkl | depart,roundabout-exit-4,exit roundabout right,arrive |
           | j,i       | jkl,ghi,ghi,ghi | depart,roundabout-exit-1,exit roundabout right,arrive |
           | j,f       | jkl,def,def,def | depart,roundabout-exit-2,exit roundabout right,arrive |
           | j,c       | jkl,abc,abc,abc | depart,roundabout-exit-3,exit roundabout right,arrive |

     Scenario: Mixed Entry and Exit - clockwise order
        Given the node map
           """
             c   a
           j   b   f
             k   e
           l   h   d
             g   i
           """

        And the ways
           | nodes | junction   | oneway |
           | abc   |            | yes    |
           | def   |            | yes    |
           | ghi   |            | yes    |
           | jkl   |            | yes    |
           | behkb | roundabout | yes    |

        When I route I should get
           | waypoints | route           | turns                                                |
           | a,c       | abc,abc,abc,abc | depart,roundabout-exit-4,exit roundabout left,arrive |
           | a,l       | abc,jkl,jkl,jkl | depart,roundabout-exit-3,exit roundabout left,arrive |
           | a,i       | abc,ghi,ghi,ghi | depart,roundabout-exit-2,exit roundabout left,arrive |
           | a,f       | abc,def,def,def | depart,roundabout-exit-1,exit roundabout left,arrive |
           | d,f       | def,def,def,def | depart,roundabout-exit-4,exit roundabout left,arrive |
           | d,c       | def,abc,abc,abc | depart,roundabout-exit-3,exit roundabout left,arrive |
           | d,l       | def,jkl,jkl,jkl | depart,roundabout-exit-2,exit roundabout left,arrive |
           | d,i       | def,ghi,ghi,ghi | depart,roundabout-exit-1,exit roundabout left,arrive |
           | g,i       | ghi,ghi,ghi,ghi | depart,roundabout-exit-4,exit roundabout left,arrive |
           | g,f       | ghi,def,def,def | depart,roundabout-exit-3,exit roundabout left,arrive |
           | g,c       | ghi,abc,abc,abc | depart,roundabout-exit-2,exit roundabout left,arrive |
           | g,l       | ghi,jkl,jkl,jkl | depart,roundabout-exit-1,exit roundabout left,arrive |
           | j,l       | jkl,jkl,jkl,jkl | depart,roundabout-exit-4,exit roundabout left,arrive |
           | j,i       | jkl,ghi,ghi,ghi | depart,roundabout-exit-3,exit roundabout left,arrive |
           | j,f       | jkl,def,def,def | depart,roundabout-exit-2,exit roundabout left,arrive |
           | j,c       | jkl,abc,abc,abc | depart,roundabout-exit-1,exit roundabout left,arrive |

    Scenario: Mixed Entry and Exit - segregated roads, different names
        Given the node map
           """
               a   c

           l     b     d
               k   e
           j     h     f

               i   g
           """

        And the ways
           | nodes | junction   | oneway |
           | ab    |            | yes    |
           | bc    |            | yes    |
           | de    |            | yes    |
           | ef    |            | yes    |
           | gh    |            | yes    |
           | hi    |            | yes    |
           | jk    |            | yes    |
           | kl    |            | yes    |
           | bkheb | roundabout | yes    |

        When I route I should get
           | waypoints | route       | turns                                                 |
           | a,c       | ab,bc,bc,bc | depart,roundabout-exit-4,exit roundabout right,arrive |
           | a,l       | ab,kl,kl,kl | depart,roundabout-exit-1,exit roundabout right,arrive |
           | a,i       | ab,hi,hi,hi | depart,roundabout-exit-2,exit roundabout right,arrive |
           | a,f       | ab,ef,ef,ef | depart,roundabout-exit-3,exit roundabout right,arrive |
           | d,f       | de,ef,ef,ef | depart,roundabout-exit-4,exit roundabout right,arrive |
           | d,c       | de,bc,bc,bc | depart,roundabout-exit-1,exit roundabout right,arrive |
           | d,l       | de,kl,kl,kl | depart,roundabout-exit-2,exit roundabout right,arrive |
           | d,i       | de,hi,hi,hi | depart,roundabout-exit-3,exit roundabout right,arrive |
           | g,i       | gh,hi,hi,hi | depart,roundabout-exit-4,exit roundabout right,arrive |
           | g,f       | gh,ef,ef,ef | depart,roundabout-exit-1,exit roundabout right,arrive |
           | g,c       | gh,bc,bc,bc | depart,roundabout-exit-2,exit roundabout right,arrive |
           | g,l       | gh,kl,kl,kl | depart,roundabout-exit-3,exit roundabout right,arrive |
           | j,l       | jk,kl,kl,kl | depart,roundabout-exit-4,exit roundabout right,arrive |
           | j,i       | jk,hi,hi,hi | depart,roundabout-exit-1,exit roundabout right,arrive |
           | j,f       | jk,ef,ef,ef | depart,roundabout-exit-2,exit roundabout right,arrive |
           | j,c       | jk,bc,bc,bc | depart,roundabout-exit-3,exit roundabout right,arrive |

    Scenario: Motorway Roundabout
    #See 39.933742 -75.082345
        Given the node map
            """
                    l       a   i


                        b
                  c

                            h
            n

                d               j

                    m     g


                e   f
            """

        And the ways
            | nodes | junction   | name     | highway    | oneway | ref    |
            | ab    |            | crescent | trunk      | yes    | US 130 |
            | bcd   | roundabout | crescent | trunk      | yes    | US 130 |
            | de    |            | crescent | trunk      | yes    | US 130 |
            | fg    |            | crescent | trunk      | yes    | US 130 |
            | gh    | roundabout | crescent | trunk      | yes    | US 130 |
            | hi    |            | crescent | trunk      | yes    | US 130 |
            | jh    |            |          | trunk_link | yes    | NJ 38  |
            | hb    | roundabout |          | trunk_link | yes    | NJ 38  |
            | bl    |            |          | trunk_link | yes    | NJ 38  |
            | cnd   |            | kaighns  | trunk_link | yes    |        |
            | dmg   | roundabout |          | trunk_link | yes    |        |

        When I route I should get
            | waypoints | route                               | turns                                                    | ref                         |
            | a,e       | crescent,crescent,crescent,crescent | depart,roundabout-exit-3,exit roundabout straight,arrive | US 130,US 130,US 130,US 130 |
            | j,l       | ,,,                                 | depart,roundabout-exit-2,exit roundabout straight,arrive | NJ 38,NJ 38,NJ 38,NJ 38     |

    # this test previously only passed by accident. We need to handle throughabouts correctly, since staying on massachusetts is actually
    # the desired setting. Rotary instructions here are not wanted but rather no instruction at all to go through the roundabout (or add
    # a throughabout instruction)
    # see https://github.com/Project-OSRM/osrm-backend/issues/3142
    Scenario: Double Roundabout with through-lane
    #http://map.project-osrm.org/?z=18&center=38.911752%2C-77.048667&loc=38.912003%2C-77.050831&loc=38.909277%2C-77.042516&hl=en&alt=0
        Given the node map
            """
                    o                       n
                   .e.                     _j_.
                  /   '.                  /    \
                 /      q__             /       |
            a---b       |  >s---f-------g       i---k
                .       r''     |       .' . .p'|
                 .      |       t        .     .'
                   'c---d                  'h'
                    l                       m
            """

        And the nodes
            | node | highway         |
            | i    | traffic_signals |

        And the ways
            | nodes   | junction   | name            | oneway |
            | bcdrqeb | roundabout | sheridan circle | yes    |
            | ghi     | roundabout | dupont circle   | yes    |
            | ij      | roundabout | dupont circle   | yes    |
            | jg      | roundabout | dupont circle   | yes    |
            | ab      |            | massachusetts   | no     |
            | gp      |            | massachusetts   | no     |
            | pi      |            | massachusetts   | no     |
            | sfg     |            | massachusetts   | no     |
            | ik      |            | massachusetts   | no     |
            | cl      |            | 23rd street     | no     |
            | oe      |            | r street        | no     |
            | jn      |            | new hampshire   | no     |
            | mh      |            | new hampshire   | yes    |
            | rsq     |            | massachusetts   | yes    |
            | ft      |            | suppressed      | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | pi       | ij     | i        | no_left_turn  |

        When I route I should get
            | waypoints | route                                                   | turns                                                  |
            | a,k       | massachusetts,massachusetts,massachusetts,massachusetts | depart,sheridan circle-exit-2,exit rotary right,arrive |

    #2856 - http://www.openstreetmap.org/#map=19/47.23318/-1.56563
    Scenario: Linked Roundabouts
        Given the node map
            """
                                    x
            u                     r

                t
                              s
              v     i   h   g
                                q

                j               f


                a               e


                    b   c   d   p

              m               n
                  l







              k



            w                   o
            """

        And the ways
            | nodes | junction   | name | highway   | oneway |
            | abija | roundabout | egg  | primary   | yes    |
            | defgd | roundabout | egg  | primary   | yes    |
            | bcd   | roundabout | egg  | primary   | yes    |
            | ghi   |            | egg  | primary   | yes    |
            | amklb |            | ll   | primary   | yes    |
            | wk    |            | ll   | primary   | no     |
            | dnope |            | lr   | secondary | yes    |
            | fqrsg |            | tr   | primary   | yes    |
            | rx    |            | tr   | primary   | no     |
            | ituvj |            | tl   | primary   | yes    |

        And the nodes
            | node | highway  |
            | c    | give_way |
            | h    | give_way |

        When I route I should get
            | waypoints | route            | turns                                                | locations |
            # since we cannot handle these invalid roundabout tags yet, we cannot output roundabout taggings. This will hopefully change some day
            #| w,x       | ll,egg,egg,tr,tr | depart,roundabout-exit-1,roundabout-exit-2,arrive       |
            | w,x       | ll,egg,egg,tr,tr | depart,turn right,continue left,turn straight,arrive | w,b,d,f,x |

    Scenario: Use Lane in Roundabout
        Given the node map
            """
                    . i  . . . .. .
                  .'                '.
            a - b.                    f - g
                 .                    |
                  '. 1               /
                     '.             /
                j - - - c .       . e
                            ' d '
                                 '  h
            """

        #using roundabout as name, we can ignore whether we see a roundabout or a rotary here. Cucumber output will be the same
        And the ways
            | nodes  | junction   | name          | oneway | turn:lanes:forward    |
            | ba     |            | left-out      | yes    |                       |
            | jc     |            | left-in       | yes    |                       |
            | dh     |            | right-bot-out | yes    |                       |
            | fg     |            | right-top-out | yes    |                       |
            | bc     | roundabout | roundabout    | yes    | left;through\|through |
            | cdefib | roundabout | roundabout    | yes    |                       |

        When I route I should get
            | waypoints | route                                  | turns                              |
            | 1,h       | roundabout,right-bot-out,right-bot-out | depart,exit rotary straight,arrive |

    @3254
    Scenario: Driving up to and through a roundabout
        Given the node map
            """
              g            a
              |          /   \
            e-f- - - - b      d - - - h
              |          \   /
              i            c
                           |
                           k
            """

        And the ways
            | nodes | junction   | name       | highway     |
            | abcda | roundabout | rotary     | residential |
            | gfi   |            | side       | residential |
            | efb   |            | left       | residential |
            | dh    |            | right      | residential |
            | ck    |            | bottom     | residential |

        When I route I should get
            | waypoints | route                  | turns                                         |
            | e,h       | left,right,right,right | depart,rotary-exit-2,exit rotary right,arrive |

    @3254
    Scenario: Driving up to and through a roundabout
        Given the node map
            """
              g       a
              |     /   \
            e-f - b      d - - - h
              |     \   /
              i       c
                      |
                      k
            """

        And the ways
            | nodes | junction   | name       | highway     |
            | abcda | roundabout | rotary     | residential |
            | gfi   |            | side       | residential |
            | efb   |            | left       | residential |
            | dh    |            | right      | residential |
            | ck    |            | bottom     | residential |

        When I route I should get
            | waypoints | route                  | turns                                         |
            | e,h       | left,right,right,right | depart,rotary-exit-2,exit rotary right,arrive |

    @3361
    Scenario: Bersarinplatz (Not a Roundabout)
        Given the node map
            """
                   a    n

                   b     m

                c          l

            d  e         j   k

                f     h

                g     i
            """

        And the ways
            | nodes     | junction   | name                 | ref   | highway     | oneway |
            | ab        |            | Petersburger Strasse | B 96a | primary     | yes    |
            | bc        | circular   | Bersarinplatz        | B 96a | primary     |        |
            | ce        | circular   | Bersarinplatz        | B 96a | primary     |        |
            | ed        |            | Weidenweg            |       | residential |        |
            | ef        | circular   | Bersarinplatz        | B 96a | primary     |        |
            | fg        |            | Petersburger Strasse | B 96a | primary     | yes    |
            | fh        | circular   | Bersarinplatz        |       | secondary   |        |
            | ih        |            | Petersburger Strasse | B 96a | primary     | yes    |
            | hj        | circular   | Bersarinplatz        |       | secondary   |        |
            | jk        |            | Rigaer Strasse       |       | residential |        |
            | jl        | circular   | Bersarinplatz        |       | secondary   |        |
            | lm        | circular   | Bersarinplatz        |       | secondary   |        |
            | mb        | circular   | Bersarinplatz        |       | secondary   |        |
            | mn        |            | Petersburger Strasse | B 96a | primary     | yes    |

        When I route I should get
            | waypoints | route                                                                               | turns                                                   |
            | a,g       | Petersburger Strasse,Petersburger Strasse,Petersburger Strasse,Petersburger Strasse | depart,Bersarinplatz-exit-2,exit rotary straight,arrive |
            | d,g       | Weidenweg,Petersburger Strasse,Petersburger Strasse,Petersburger Strasse            | depart,Bersarinplatz-exit-1,exit rotary straight,arrive |
            | i,k       | Petersburger Strasse,Rigaer Strasse,Rigaer Strasse,Rigaer Strasse                   | depart,Bersarinplatz-exit-1,exit rotary right,arrive    |
            | i,n       | Petersburger Strasse,Petersburger Strasse,Petersburger Strasse,Petersburger Strasse | depart,Bersarinplatz-exit-2,exit rotary straight,arrive |
            | i,d       | Petersburger Strasse,Weidenweg,Weidenweg,Weidenweg                                  | depart,Bersarinplatz-exit-3,exit rotary right,arrive    |
            | i,g       | Petersburger Strasse,Petersburger Strasse,Petersburger Strasse,Petersburger Strasse | depart,Bersarinplatz-exit-4,exit rotary straight,arrive |

    @turboroundabout
    # http://www.openstreetmap.org/?mlat=48.782118&mlon=8.194456&zoom=16#map=19/48.78216/8.19457
    Scenario: Turboroundabout, Baden-Baden
        Given the node map
            """
                a p
                b o
            d c     m n
            f e     k l
                g i
                h j
            """

       And the ways
            | nodes | highway     | oneway | junction   | name          | turn:lanes                             |
            | ab    | trunk_link  | yes    |            |               |                                        |
            | bc    | trunk       | yes    | roundabout | Europaplatz   | slight_left;slight_right\|slight_right |
            | cd    | trunk       | yes    |            | Europastrasse |                                        |
            | ce    | trunk       | yes    | roundabout | Europaplatz   |                                        |
            | fe    | trunk       | yes    |            | Europastrasse |                                        |
            | eg    | trunk       | yes    | roundabout | Europaplatz   |                                        |
            | gh    | residential | yes    |            | Allee Cite    |                                        |
            | gi    | trunk       | yes    | roundabout | Europaplatz   |                                        |
            | ji    | residential | yes    |            | Allee Cite    |                                        |
            | ik    | trunk       | yes    | roundabout | Europaplatz   | slight_left;slight_right\|slight_right |
            | kl    | trunk       | yes    |            | Europastrasse |                                        |
            | km    | trunk       | yes    | roundabout | Europaplatz   |                                        |
            | nm    | trunk       | yes    |            | Europastrasse |                                        |
            | mo    | trunk       | yes    | roundabout | Europaplatz   |                                        |
            | op    | trunk_link  | yes    |            |               |                                        |
            | ob    | trunk       | yes    | roundabout | Europaplatz   |                                        |

       When I route I should get
           | waypoints | route                                      | turns                                              |
           | a,d       | ,Europastrasse,Europastrasse,Europastrasse | depart,Europaplatz-exit-1,exit rotary right,arrive |
           | a,h       | ,Allee Cite,Allee Cite,Allee Cite          | depart,Europaplatz-exit-2,exit rotary right,arrive |
           | a,l       | ,Europastrasse,Europastrasse,Europastrasse | depart,Europaplatz-exit-3,exit rotary right,arrive |
           | a,p       | ,,,                                        | depart,Europaplatz-exit-4,exit rotary right,arrive |

    @turboroundabout
    # http://www.openstreetmap.org/?mlat=50.180039&mlon=8.474939&zoom=16#map=19/50.17999/8.47506
    Scenario: Turboroundabout, Königstein im Taunus
        Given the node map
            """
                a
                b   w t   v
              c         s u
              d           r
            f e         q
              g         p
            h   i     n
              j   k m
                    l o
            """

       And the ways
            | nodes | highway      | oneway | junction   | name                         | turn:lanes                         |
            | ab    | primary      | yes    |            | Le-Cannet-Rocheville-Strasse |                                    |
            | wa    | primary      | yes    |            | Le-Cannet-Rocheville-Strasse |                                    |
            | bc    | primary      | yes    | roundabout |                              | through\|through;right             |
            | cd    | primary      | yes    | roundabout |                              | through\|through\|right;through    |
            | df    | tertiary     | yes    |            | Frankfurter Strasse          |                                    |
            | de    | primary      | yes    | roundabout |                              | through\|through\|right;through    |
            | fe    | tertiary     | yes    |            | Frankfurter Strasse          |                                    |
            | eg    | primary      | yes    | roundabout |                              | through\|through\|right;through    |
            | gh    | primary      | yes    |            | Bischof-Kaller-Strasse       |                                    |
            | gi    | primary      | yes    | roundabout |                              | left\|through;slight_left\|through |
            | ji    | primary      | yes    |            | Bischof-Kaller-Strasse       |                                    |
            | ik    | primary      | yes    | roundabout |                              | left\|through;slight_left\|through |
            | km    | primary      | yes    | roundabout |                              |                                    |
            | kl    | primary      | yes    |            | Sodener Strasse              |                                    |
            | mn    | primary      | yes    | roundabout |                              | through\|through;right             |
            | on    | primary      | yes    |            | Sodener Strasse              |                                    |
            | np    | primary      | yes    | roundabout |                              | through\|through;right             |
            | pq    | primary      | yes    | roundabout |                              | through\|through\|right;through    |
            | qr    | primary      | yes    |            |                              |                                    |
            | qs    | primary      | yes    | roundabout |                              |                                    |
            | us    | primary_link | yes    |            |                              |                                    |
            | st    | primary      | yes    | roundabout |                              |                                    |
            | vt    | primary      | yes    |            |                              |                                    |
            | tw    | primary      | yes    | roundabout |                              | left\|left\|right\|right           |
            | wa    | primary      | yes    |            | Le-Cannet-Rocheville-Strasse |                                    |
            | wb    | primary      | yes    | roundabout |                              | through\|through;right             |

       When I route I should get
           | waypoints | route                                                                                             | turns                                                 |
           | a,w       | Le-Cannet-Rocheville-Strasse,,                                                                    | depart,roundabout-exit-undefined,arrive               |
           | a,r       | Le-Cannet-Rocheville-Strasse,,,                                                                   | depart,roundabout-exit-4,exit roundabout right,arrive |
           | a,f       | Le-Cannet-Rocheville-Strasse,Frankfurter Strasse,Frankfurter Strasse,Frankfurter Strasse          | depart,roundabout-exit-1,exit roundabout right,arrive |
           | a,h       | Le-Cannet-Rocheville-Strasse,Bischof-Kaller-Strasse,Bischof-Kaller-Strasse,Bischof-Kaller-Strasse | depart,roundabout-exit-2,exit roundabout right,arrive |
           | u,r       | ,,,                                                                                               | depart,roundabout-exit-5,exit roundabout right,arrive |
           | j,h       | Bischof-Kaller-Strasse,Bischof-Kaller-Strasse,Bischof-Kaller-Strasse,Bischof-Kaller-Strasse       | depart,roundabout-exit-5,exit roundabout right,arrive |
           | n,m       | ,                                                                                                 | depart,arrive                                         |

    @turboroundabout
    # http://www.openstreetmap.org/?mlat=47.57723&mlon=7.796765&zoom=16#map=19/47.57720/7.79711
    Scenario: Turboroundabout, Rheinfelden (Baden)
        Given the node map
            """
            r             w
                a   l k
              b         j
              c
              d         i
            s   e f g h   v

                  t u
            """

       And the ways
            | nodes | highway      | oneway | junction   |
            | ar    | secondary    | yes    |            |
            | ab    | primary      | yes    | roundabout |
            | rb    | secondary    | yes    |            |
            | bc    | primary      | yes    | roundabout |
            | cd    | primary      | yes    | roundabout |
            | ds    | primary      | yes    |            |
            | se    | primary      | yes    |            |
            | de    | primary      | yes    | roundabout |
            | ef    | primary      | yes    | roundabout |
            | ft    | unclassified | yes    |            |
            | fg    | primary      | yes    | roundabout |
            | ug    | unclassified | yes    |            |
            | gh    | primary      | yes    | roundabout |
            | hv    | primary      | yes    |            |
            | hi    | primary      | yes    | roundabout |
            | vi    | primary      | yes    |            |
            | ij    | primary      | yes    | roundabout |
            | jw    | tertiary     | yes    |            |
            | jk    | primary      | yes    | roundabout |
            | wk    | tertiary     | yes    |            |
            | kl    | primary      | yes    | roundabout |
            | la    | primary      | yes    | roundabout |

       When I route I should get
           | waypoints | route       | turns                                                        |
           | w,r       | wk,ar,ar,ar | depart,roundabout-exit-1,exit roundabout slight right,arrive |
           | w,s       | wk,ds,ds,ds | depart,roundabout-exit-2,exit roundabout right,arrive        |
           | w,t       | wk,ft,ft,ft | depart,roundabout-exit-3,exit roundabout right,arrive        |
           | w,v       | wk,hv,hv,hv | depart,roundabout-exit-4,exit roundabout straight,arrive     |
           | u,v       | ug,hv,hv,hv | depart,roundabout-exit-1,exit roundabout straight,arrive     |
           | u,w       | ug,jw,jw,jw | depart,roundabout-exit-2,exit roundabout slight right,arrive |
           | u,r       | ug,ar,ar,ar | depart,roundabout-exit-3,exit roundabout slight right,arrive |
           | u,s       | ug,ds,ds,ds | depart,roundabout-exit-4,exit roundabout right,arrive        |
           | u,t       | ug,ft,ft,ft | depart,roundabout-exit-5,exit roundabout right,arrive        |

    @3762
    Scenario: Only Enter
        Given the node map
            """
                  a
                  b
           i   c     e ~ ~ ~ f - h
                j d
              k   g
            """

        And the ways
            | nodes  | junction   | route |
            | ab     |            |       |
            | ef     |            | ferry |
            | fh     |            |       |
            | dg     |            |       |
            | ic     |            |       |
            | jk     |            |       |
            | bcjdeb | roundabout |       |

        When I route I should get
            | waypoints | route          | turns                                                                              |
            | a,h       | ab,ef,ef,fh,fh | depart,roundabout-exit-4,exit roundabout slight right,notification straight,arrive |


     Scenario: Drive through roundabout
        Given a grid size of 5 meters
        Given the node map
           """
                . a .
              .       .
            b     e --- d ---- f
              .        .
                 .c.
               g     h
           """

        And the ways
           | nodes | junction   | oneway |
           | abcda | roundabout | yes    |
           | edf   |            |        |
           | gch   |            | yes    |

        When I route I should get
           | waypoints | bearings | route           | turns                                                        |
           | e,f       | 90 90    | edf,edf         | depart,arrive                                                |
           | e,h       | 90 130   | edf,gch,gch,gch | depart,roundabout-exit-2,exit roundabout straight,arrive     |
           | g,f       | 50 90    | gch,edf,edf,edf | depart,roundabout-exit-2,exit roundabout slight right,arrive |
           | g,h       | 50 130   | gch,gch,gch     | depart,exit roundabout right,arrive                          |
           | e,e       | 90 270   | edf,edf,edf,edf | depart,roundabout-exit-3,exit roundabout sharp left,arrive   |

    Scenario: CCW and CW roundabouts with overlaps
        Given the node map
            """
            a   d          g   h

            b   c          j   i
            f   e          k   l
            """

        And the ways
            | nodes | highway  | junction   |
            | abcda | tertiary | roundabout |
            | ed    | tertiary |            |
            | af    | tertiary |            |
            | ghijg | tertiary | roundabout |
            | kg    | tertiary |            |
            | hl    | tertiary |            |

        # the turn angles here are quite strange, so we do get uturns for exiting
        When I route I should get
            | from | to | route       | turns                                                 | distance |
            | e    | f  | ed,af,af,af | depart,roundabout-exit-1,exit roundabout left,arrive  | 80m      |
            | f    | e  | af,ed,ed,ed | depart,roundabout-exit-1,exit roundabout uturn,arrive | 120m     |
            | k    | l  | kg,hl,hl,hl | depart,roundabout-exit-1,exit roundabout right,arrive | 80m      |
            | l    | k  | hl,kg,kg,kg | depart,roundabout-exit-1,exit roundabout uturn,arrive | 120m     |

    @4030 @4075
    Scenario: Service roundabout with service exits
        Given the node map
            """
            g a d f
            h b1c e
            """

        And the ways
            | nodes | highway  | junction   |
            | abcda | service  | roundabout |
            | ce    | service  |            |
            | df    | service  |            |
            | ag    | tertiary |            |
            | bh    | service  |            |

        When I route I should get
            | from | to | route       | turns                                   |
            |    1 | e  | abcda,ce,ce | depart,exit roundabout straight,arrive  |
            |    1 | f  | abcda,df,df | depart,exit roundabout right,arrive     |
            |    1 | g  | abcda,ag,ag | depart,exit roundabout straight,arrive  |
            |    1 | h  | abcda,bh,bh | depart,exit roundabout right,arrive     |

    Scenario: Collapsing a sliproad step after roundabouts
        Given the node map
            """
                  a         r          j
                ╱   ╲     ╱   ╲        │
            e——b——1——d———s     u——f——g—h——l
                ╲   ╱     ╲   ╱       `i
                  c         t          │
                  │         │          │
                  m         v          k
            """

        And the ways
            | nodes | highway       | junction   | oneway | #         |
            | abcda | tertiary      | roundabout |        | circle    |
            | ebds  | tertiary      |            |        | road      |
            | cm    | tertiary      |            |        |           |
            | ds    | tertiary      |            |        | road      |
            | rstur | tertiary      | roundabout |        | circle2   |
            | ufghl | tertiary      |            |        | road      |
            | tv    | tertiary      |            |        |           |
            | gi    | tertiary_link |            | yes    | sliproad  |
            | jhik  | tertiary      |            |        | crossroad |


        When I route I should get
            | from | to | route                      | turns                                                   | distance |
            | e    | k  | ebds,ufghl,ufghl,jhik,jhik | depart,rstur-exit-2,exit rotary right,turn right,arrive | 189.2m   |
            | 1    | k  | ebds,ufghl,ufghl,jhik,jhik | depart,rstur-exit-2,exit rotary right,turn right,arrive | 159.1m   |
