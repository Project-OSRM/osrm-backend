@routing  @guidance
Feature: Rotary

    Background:
        Given the profile "car"
        Given a grid size of 30 meters

    Scenario: Enter and Exit
        Given the node map
            """
                a
                b
            h g   c d
                e
                f
            """

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bgecb  | roundabout |

       When I route I should get
            | waypoints | route       | turns                                        | locations |
            | a,d       | ab,cd,cd,cd | depart,bgecb-exit-3,exit rotary right,arrive | a,?,c,d   |
            | a,f       | ab,ef,ef,ef | depart,bgecb-exit-2,exit rotary right,arrive | a,?,e,f   |
            | a,h       | ab,gh,gh,gh | depart,bgecb-exit-1,exit rotary right,arrive | a,?,g,h   |
            | d,f       | cd,ef,ef,ef | depart,bgecb-exit-3,exit rotary right,arrive | d,?,e,f   |
            | d,h       | cd,gh,gh,gh | depart,bgecb-exit-2,exit rotary right,arrive | d,?,g,h   |
            | d,a       | cd,ab,ab,ab | depart,bgecb-exit-1,exit rotary right,arrive | d,?,a,a   |
            | f,h       | ef,gh,gh,gh | depart,bgecb-exit-3,exit rotary right,arrive | f,?,g,h   |
            | f,a       | ef,ab,ab,ab | depart,bgecb-exit-2,exit rotary right,arrive | f,?,a,a   |
            | f,d       | ef,cd,cd,cd | depart,bgecb-exit-1,exit rotary right,arrive | f,?,c,d   |
            | h,a       | gh,ab,ab,ab | depart,bgecb-exit-3,exit rotary right,arrive | h,?,a,a   |
            | h,d       | gh,cd,cd,cd | depart,bgecb-exit-2,exit rotary right,arrive | h,?,c,d   |
            | h,f       | gh,ef,ef,ef | depart,bgecb-exit-1,exit rotary right,arrive | h,?,e,f   |

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
            | waypoints | route          | turns                              | locations |
            | a,c       | ab,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | a,b,c     |
            | a,e       | ab,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | a,b,e     |
            | a,g       | ab,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | a,b,g     |
            | d,e       | cd,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | d,c,e     |
            | d,g       | cd,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | d,c,g     |
            | d,b       | cd,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | d,c,b     |
            | f,g       | ef,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | f,e,g     |
            | f,b       | ef,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | f,e,b     |
            | f,c       | ef,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | f,e,c     |
            | h,b       | gh,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | h,g,b     |
            | h,c       | gh,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | h,g,c     |
            | h,e       | gh,bcegb,bcegb | depart,bcegb-exit-undefined,arrive | h,g,e     |

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
            | waypoints | route       | turns                           | locations |
            | b,d       | bcegb,cd,cd | depart,exit rotary right,arrive | b,c,d     |
            | b,f       | bcegb,ef,ef | depart,exit rotary right,arrive | b,e,f     |
            | b,h       | bcegb,gh,gh | depart,exit rotary right,arrive | b,g,h     |
            | c,f       | bcegb,ef,ef | depart,exit rotary right,arrive | c,e,f     |
            | c,h       | bcegb,gh,gh | depart,exit rotary right,arrive | c,g,h     |
            | c,a       | bcegb,ab,ab | depart,exit rotary right,arrive | c,b,a     |
            | e,h       | bcegb,gh,gh | depart,exit rotary right,arrive | e,g,h     |
            | e,a       | bcegb,ab,ab | depart,exit rotary right,arrive | e,b,a     |
            | e,d       | bcegb,cd,cd | depart,exit rotary right,arrive | e,c,d     |
            | g,a       | bcegb,ab,ab | depart,exit rotary right,arrive | g,b,a     |
            | g,d       | bcegb,cd,cd | depart,exit rotary right,arrive | g,c,d     |
            | g,f       | bcegb,ef,ef | depart,exit rotary right,arrive | g,e,f     |
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
            | waypoints | route       | turns         | locations |
            | b,c       | bcegb,bcegb | depart,arrive | b,c       |
            | b,e       | bcegb,bcegb | depart,arrive | b,e       |
            | b,g       | bcegb,bcegb | depart,arrive | b,g       |
            | c,e       | bcegb,bcegb | depart,arrive | c,e       |
            | c,g       | bcegb,bcegb | depart,arrive | c,g       |
            | c,b       | bcegb,bcegb | depart,arrive | c,b       |
            | e,g       | bcegb,bcegb | depart,arrive | e,g       |
            | e,b       | bcegb,bcegb | depart,arrive | e,b       |
            | e,c       | bcegb,bcegb | depart,arrive | e,c       |
            | g,b       | bcegb,bcegb | depart,arrive | g,b       |
            | g,c       | bcegb,bcegb | depart,arrive | g,c       |
            | g,e       | bcegb,bcegb | depart,arrive | g,e       |

     #needs to be adjusted when name-discovery works for entrys
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
            | waypoints | route           | turns                                           | locations |
            | a,c       | abc,abc,abc     | depart,exit rotary right,arrive                 | a,a,c     |
            | a,l       | abc,jkl,jkl,jkl | depart,bkheb-exit-2,exit rotary straight,arrive | a,?,j,l   |
            | a,i       | abc,ghi,ghi,ghi | depart,bkheb-exit-3,exit rotary straight,arrive | a,?,g,i   |
            | a,f       | abc,def,def,def | depart,bkheb-exit-4,exit rotary straight,arrive | a,?,d,f   |
            | d,f       | def,def,def     | depart,exit rotary right,arrive                 | d,d,f     |
            | d,c       | def,abc,abc,abc | depart,bkheb-exit-2,exit rotary straight,arrive | d,?,a,c   |
            | d,l       | def,jkl,jkl,jkl | depart,bkheb-exit-3,exit rotary straight,arrive | d,?,j,l   |
            | d,i       | def,ghi,ghi,ghi | depart,bkheb-exit-4,exit rotary straight,arrive | d,?,g,i   |
            | g,i       | ghi,ghi,ghi     | depart,exit rotary right,arrive                 | g,g,i     |
            | g,f       | ghi,def,def,def | depart,bkheb-exit-2,exit rotary straight,arrive | g,?,d,f   |
            | g,c       | ghi,abc,abc,abc | depart,bkheb-exit-3,exit rotary straight,arrive | g,?,a,c   |
            | g,l       | ghi,jkl,jkl,jkl | depart,bkheb-exit-4,exit rotary straight,arrive | g,?,j,l   |
            | j,l       | jkl,jkl,jkl     | depart,exit rotary right,arrive                 | j,j,l     |
            | j,i       | jkl,ghi,ghi,ghi | depart,bkheb-exit-2,exit rotary straight,arrive | j,?,g,i   |
            | j,f       | jkl,def,def,def | depart,bkheb-exit-3,exit rotary straight,arrive | j,?,d,f   |
            | j,c       | jkl,abc,abc,abc | depart,bkheb-exit-4,exit rotary straight,arrive | j,?,a,c   |

       Scenario: Collinear in X,Y
        Given the node map
            """
            a
            b
            c d f
            e
            """

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route       | turns                                          | locations |
            | a,e       | ab,ce,ce,ce | depart,bcdb-exit-1,exit rotary straight,arrive | a,?,c,e   |
            | a,f       | ab,df,df,df | depart,bcdb-exit-2,exit rotary straight,arrive | a,?,d,f   |

       Scenario: Collinear in X,Y
        Given the node map
            """
            a
            d
            b c f
            e
            """

        And the ways
            | nodes | junction   |
            | ad    |            |
            | bcdb  | roundabout |
            | be    |            |
            | cf    |            |

        When I route I should get
            | waypoints | route       | turns                                          | locations |
            | a,e       | ad,be,be,be | depart,bcdb-exit-1,exit rotary straight,arrive | a,?,b,e   |
            | a,f       | ad,cf,cf,cf | depart,bcdb-exit-2,exit rotary straight,arrive | a,?,c,f   |

       Scenario: Collinear in X,Y
        Given the node map
            """
            a
            c
            d b f
            e
            """

        And the ways
            | nodes | junction   |
            | ac    |            |
            | bcdb  | roundabout |
            | de    |            |
            | bf    |            |

        When I route I should get
            | waypoints | route       | turns                                          | locations |
            | a,e       | ac,de,de,de | depart,bcdb-exit-1,exit rotary straight,arrive | a,?,d,e   |
            | a,f       | ac,bf,bf,bf | depart,bcdb-exit-2,exit rotary straight,arrive | a,?,b,f   |

       Scenario: Collinear in X,Y
        Given the node map
            """
            f
            d c e
              b
              a
            """

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route       | turns                                       | locations |
            | a,e       | ab,ce,ce,ce | depart,bcdb-exit-1,exit rotary right,arrive | a,?,c,e   |
            | a,f       | ab,df,df,df | depart,bcdb-exit-2,exit rotary right,arrive | a,?,d,f   |

       Scenario: Collinear in X,Y
        Given the node map
            """
            f
            d c e
            b
            a
            """

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route       | turns                                       | locations |
            | a,e       | ab,ce,ce,ce | depart,bcdb-exit-1,exit rotary right,arrive | a,?,c,e   |
            | a,f       | ab,df,df,df | depart,bcdb-exit-2,exit rotary right,arrive | a,?,d,f   |
