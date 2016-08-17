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
           | waypoints | route       | turns                           |
           | b,d       | bcegb,cd,cd | depart,roundabout-exit-1,arrive |
           | b,f       | bcegb,ef,ef | depart,roundabout-exit-2,arrive |
           | b,h       | bcegb,gh,gh | depart,roundabout-exit-3,arrive |
           | c,f       | bcegb,ef,ef | depart,roundabout-exit-1,arrive |
           | c,h       | bcegb,gh,gh | depart,roundabout-exit-2,arrive |
           | c,a       | bcegb,ab,ab | depart,roundabout-exit-3,arrive |
           | e,h       | bcegb,gh,gh | depart,roundabout-exit-1,arrive |
           | e,a       | bcegb,ab,ab | depart,roundabout-exit-2,arrive |
           | e,d       | bcegb,cd,cd | depart,roundabout-exit-3,arrive |
           | g,a       | bcegb,ab,ab | depart,roundabout-exit-1,arrive |
           | g,d       | bcegb,cd,cd | depart,roundabout-exit-2,arrive |
           | g,f       | bcegb,ef,ef | depart,roundabout-exit-3,arrive |
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
           | waypoints | route       | turns                           |
           | a,c       | abc,abc,abc | depart,roundabout-exit-1,arrive |
           | a,l       | abc,jkl,jkl | depart,roundabout-exit-2,arrive |
           | a,i       | abc,ghi,ghi | depart,roundabout-exit-3,arrive |
           | a,f       | abc,def,def | depart,roundabout-exit-4,arrive |
           | d,f       | def,def,def | depart,roundabout-exit-1,arrive |
           | d,c       | def,abc,abc | depart,roundabout-exit-2,arrive |
           | d,l       | def,jkl,jkl | depart,roundabout-exit-3,arrive |
           | d,i       | def,ghi,ghi | depart,roundabout-exit-4,arrive |
           | g,i       | ghi,ghi,ghi | depart,roundabout-exit-1,arrive |
           | g,f       | ghi,def,def | depart,roundabout-exit-2,arrive |
           | g,c       | ghi,abc,abc | depart,roundabout-exit-3,arrive |
           | g,l       | ghi,jkl,jkl | depart,roundabout-exit-4,arrive |
           | j,l       | jkl,jkl,jkl | depart,roundabout-exit-1,arrive |
           | j,i       | jkl,ghi,ghi | depart,roundabout-exit-2,arrive |
           | j,f       | jkl,def,def | depart,roundabout-exit-3,arrive |
           | j,c       | jkl,abc,abc | depart,roundabout-exit-4,arrive |

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
           | waypoints | route       | turns                           |
           | a,c       | abc,abc,abc | depart,roundabout-exit-4,arrive |
           | a,l       | abc,jkl,jkl | depart,roundabout-exit-1,arrive |
           | a,i       | abc,ghi,ghi | depart,roundabout-exit-2,arrive |
           | a,f       | abc,def,def | depart,roundabout-exit-3,arrive |
           | d,f       | def,def,def | depart,roundabout-exit-4,arrive |
           | d,c       | def,abc,abc | depart,roundabout-exit-1,arrive |
           | d,l       | def,jkl,jkl | depart,roundabout-exit-2,arrive |
           | d,i       | def,ghi,ghi | depart,roundabout-exit-3,arrive |
           | g,i       | ghi,ghi,ghi | depart,roundabout-exit-4,arrive |
           | g,f       | ghi,def,def | depart,roundabout-exit-1,arrive |
           | g,c       | ghi,abc,abc | depart,roundabout-exit-2,arrive |
           | g,l       | ghi,jkl,jkl | depart,roundabout-exit-3,arrive |
           | j,l       | jkl,jkl,jkl | depart,roundabout-exit-4,arrive |
           | j,i       | jkl,ghi,ghi | depart,roundabout-exit-1,arrive |
           | j,f       | jkl,def,def | depart,roundabout-exit-2,arrive |
           | j,c       | jkl,abc,abc | depart,roundabout-exit-3,arrive |

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
           | waypoints | route    | turns                           |
           | a,c       | ab,bc,bc | depart,roundabout-exit-4,arrive |
           | a,l       | ab,kl,kl | depart,roundabout-exit-1,arrive |
           | a,i       | ab,hi,hi | depart,roundabout-exit-2,arrive |
           | a,f       | ab,ef,ef | depart,roundabout-exit-3,arrive |
           | d,f       | de,ef,ef | depart,roundabout-exit-4,arrive |
           | d,c       | de,bc,bc | depart,roundabout-exit-1,arrive |
           | d,l       | de,kl,kl | depart,roundabout-exit-2,arrive |
           | d,i       | de,hi,hi | depart,roundabout-exit-3,arrive |
           | g,i       | gh,hi,hi | depart,roundabout-exit-4,arrive |
           | g,f       | gh,ef,ef | depart,roundabout-exit-1,arrive |
           | g,c       | gh,bc,bc | depart,roundabout-exit-2,arrive |
           | g,l       | gh,kl,kl | depart,roundabout-exit-3,arrive |
           | j,l       | jk,kl,kl | depart,roundabout-exit-4,arrive |
           | j,i       | jk,hi,hi | depart,roundabout-exit-1,arrive |
           | j,f       | jk,ef,ef | depart,roundabout-exit-2,arrive |
           | j,c       | jk,bc,bc | depart,roundabout-exit-3,arrive |

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
            | waypoints | route                                                 | turns                           | ref                     |
            | a,e       | crescent,crescent,crescent                            | depart,roundabout-exit-3,arrive | US 130,US 130,US 130    |
            | j,l       | ,,                                                    | depart,roundabout-exit-2,arrive | NJ 38,NJ 38,NJ 38       |

    Scenario: Double Roundabout with through-lane
    #http://map.project-osrm.org/?z=18&center=38.911752%2C-77.048667&loc=38.912003%2C-77.050831&loc=38.909277%2C-77.042516&hl=en&alt=0
        Given the node map
            """
                    o                       n
                    e                       j

                        q
            a   b           s   f       g       i   k
                        r                     p
                                t
                    c   d                   h
                    l                       m
            """

        And the nodes
            | node | highway         |
            | i    | traffic_signals |

        And the ways
            | nodes   | junction   | name            | oneway |
            | bcdrqeb | roundabout | sheridan circle | yes    |
            | ghi     | roundabout | dupont circle   | yes    |
            | ijg     | roundabout | dupont circle   | yes    |
            | ab      |            | massachusetts   | no     |
            | sfgpik  |            | massachusetts   | no     |
            | cl      |            | 23rd street     | no     |
            | oe      |            | r street        | no     |
            | jn      |            | new hampshire   | no     |
            | mh      |            | new hampshire   | yes    |
            | rsq     |            | massachusetts   | yes    |
            | ft      |            | suppressed      | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | sfgpik   | ijg    | i        | no_left_turn  |

        When I route I should get
            | waypoints | route                                                   | turns                                                     |
            | a,k       | massachusetts,massachusetts,massachusetts,massachusetts | depart,sheridan circle-exit-2,dupont circle-exit-1,arrive |

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
            | waypoints | route            | turns                                                |
            # since we cannot handle these invalid roundabout tags yet, we cannout output roundabout taggings. This will hopefully change some day
            #| w,x       | ll,egg,egg,tr,tr | depart,roundabout-exit-1,roundabout-exit-2,arrive       |
            | w,x       | ll,egg,egg,tr,tr | depart,turn right,continue left,turn straight,arrive |

