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
           | waypoints | route       | turns                                        |
           | a,d       | ab,cd,cd,cd | depart,bgecb-exit-3,exit rotary right,arrive |
           | a,f       | ab,ef,ef,ef | depart,bgecb-exit-2,exit rotary right,arrive |
           | a,h       | ab,gh,gh,gh | depart,bgecb-exit-1,exit rotary right,arrive |
           | d,f       | cd,ef,ef,ef | depart,bgecb-exit-3,exit rotary right,arrive |
           | d,h       | cd,gh,gh,gh | depart,bgecb-exit-2,exit rotary right,arrive |
           | d,a       | cd,ab,ab,ab | depart,bgecb-exit-1,exit rotary right,arrive |
           | f,h       | ef,gh,gh,gh | depart,bgecb-exit-3,exit rotary right,arrive |
           | f,a       | ef,ab,ab,ab | depart,bgecb-exit-2,exit rotary right,arrive |
           | f,d       | ef,cd,cd,cd | depart,bgecb-exit-1,exit rotary right,arrive |
           | h,a       | gh,ab,ab,ab | depart,bgecb-exit-3,exit rotary right,arrive |
           | h,d       | gh,cd,cd,cd | depart,bgecb-exit-2,exit rotary right,arrive |
           | h,f       | gh,ef,ef,ef | depart,bgecb-exit-1,exit rotary right,arrive |

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
           | waypoints | route          | turns                              |
           | a,c       | ab,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | a,e       | ab,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | a,g       | ab,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | d,e       | cd,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | d,g       | cd,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | d,b       | cd,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | f,g       | ef,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | f,b       | ef,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | f,c       | ef,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | h,b       | gh,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | h,c       | gh,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |
           | h,e       | gh,bcegb,bcegb | depart,bcegb-exit-undefined,arrive |

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
           | b,d       | bcegb,cd,cd | depart,exit rotary right,arrive |
           | b,f       | bcegb,ef,ef | depart,exit rotary right,arrive |
           | b,h       | bcegb,gh,gh | depart,exit rotary right,arrive |
           | c,f       | bcegb,ef,ef | depart,exit rotary right,arrive |
           | c,h       | bcegb,gh,gh | depart,exit rotary right,arrive |
           | c,a       | bcegb,ab,ab | depart,exit rotary right,arrive |
           | e,h       | bcegb,gh,gh | depart,exit rotary right,arrive |
           | e,a       | bcegb,ab,ab | depart,exit rotary right,arrive |
           | e,d       | bcegb,cd,cd | depart,exit rotary right,arrive |
           | g,a       | bcegb,ab,ab | depart,exit rotary right,arrive |
           | g,d       | bcegb,cd,cd | depart,exit rotary right,arrive |
           | g,f       | bcegb,ef,ef | depart,exit rotary right,arrive |
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
           | waypoints | route           | turns                                           |
           | a,c       | abc,abc,abc     | depart,exit rotary right,arrive                 |
           | a,l       | abc,jkl,jkl,jkl | depart,bkheb-exit-2,exit rotary straight,arrive |
           | a,i       | abc,ghi,ghi,ghi | depart,bkheb-exit-3,exit rotary straight,arrive |
           | a,f       | abc,def,def,def | depart,bkheb-exit-4,exit rotary straight,arrive |
           | d,f       | def,def,def     | depart,exit rotary right,arrive                 |
           | d,c       | def,abc,abc,abc | depart,bkheb-exit-2,exit rotary straight,arrive |
           | d,l       | def,jkl,jkl,jkl | depart,bkheb-exit-3,exit rotary straight,arrive |
           | d,i       | def,ghi,ghi,ghi | depart,bkheb-exit-4,exit rotary straight,arrive |
           | g,i       | ghi,ghi,ghi     | depart,exit rotary right,arrive                 |
           | g,f       | ghi,def,def,def | depart,bkheb-exit-2,exit rotary straight,arrive |
           | g,c       | ghi,abc,abc,abc | depart,bkheb-exit-3,exit rotary straight,arrive |
           | g,l       | ghi,jkl,jkl,jkl | depart,bkheb-exit-4,exit rotary straight,arrive |
           | j,l       | jkl,jkl,jkl     | depart,exit rotary right,arrive                 |
           | j,i       | jkl,ghi,ghi,ghi | depart,bkheb-exit-2,exit rotary straight,arrive |
           | j,f       | jkl,def,def,def | depart,bkheb-exit-3,exit rotary straight,arrive |
           | j,c       | jkl,abc,abc,abc | depart,bkheb-exit-4,exit rotary straight,arrive |

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
            | waypoints | route       | turns                                          |
            | a,e       | ab,ce,ce,ce | depart,bcdb-exit-1,exit rotary straight,arrive |
            | a,f       | ab,df,df,df | depart,bcdb-exit-2,exit rotary straight,arrive |

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
            | waypoints | route       | turns                                          |
            | a,e       | ad,be,be,be | depart,bcdb-exit-1,exit rotary straight,arrive |
            | a,f       | ad,cf,cf,cf | depart,bcdb-exit-2,exit rotary straight,arrive |

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
            | waypoints | route       | turns                                          |
            | a,e       | ac,de,de,de | depart,bcdb-exit-1,exit rotary straight,arrive |
            | a,f       | ac,bf,bf,bf | depart,bcdb-exit-2,exit rotary straight,arrive |

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
            | waypoints | route       | turns                                       |
            | a,e       | ab,ce,ce,ce | depart,bcdb-exit-1,exit rotary right,arrive |
            | a,f       | ab,df,df,df | depart,bcdb-exit-2,exit rotary right,arrive |

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
            | waypoints | route       | turns                                       |
            | a,e       | ab,ce,ce,ce | depart,bcdb-exit-1,exit rotary right,arrive |
            | a,f       | ab,df,df,df | depart,bcdb-exit-2,exit rotary right,arrive |
