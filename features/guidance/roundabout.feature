@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Enter and Exit
        Given the node map
            |   |   | a |   |   |
            |   |   | b |   |   |
            | h | g |   | c | d |
            |   |   | e |   |   |
            |   |   | f |   |   |

       And the ways
            | nodes  | junction   |
            | ab     |            |
            | cd     |            |
            | ef     |            |
            | gh     |            |
            | bgecb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                           |
           | a,d       | ab,cd,cd | depart,roundabout-exit-3,arrive |
           | a,f       | ab,ef,ef | depart,roundabout-exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout-exit-1,arrive |
           | d,f       | cd,ef,ef | depart,roundabout-exit-3,arrive |
           | d,h       | cd,gh,gh | depart,roundabout-exit-2,arrive |
           | d,a       | cd,ab,ab | depart,roundabout-exit-1,arrive |
           | f,h       | ef,gh,gh | depart,roundabout-exit-3,arrive |
           | f,a       | ef,ab,ab | depart,roundabout-exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout-exit-1,arrive |
           | h,a       | gh,ab,ab | depart,roundabout-exit-3,arrive |
           | h,d       | gh,cd,cd | depart,roundabout-exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout-exit-1,arrive |

    Scenario: Only Enter
        Given the node map
            |   |   | a |   |   |
            |   |   | b |   |   |
            | d | c |   | g | h |
            |   |   | e |   |   |
            |   |   | f |   |   |

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

    Scenario: Only Exit
        Given the node map
            |   |   | a |   |   |
            |   |   | b |   |   |
            | d | c |   | g | h |
            |   |   | e |   |   |
            |   |   | f |   |   |

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
            |   |   | a |   |   |
            |   |   | b |   |   |
            | d | c |   | g | h |
            |   |   | e |   |   |
            |   |   | f |   |   |

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
           |   | c |   | a |   |
           | j |   | b |   | f |
           |   | k |   | e |   |
           | l |   | h |   | d |
           |   | g |   | i |   |

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
           |   |   | a |   | c |   |   |
           |   |   |   |   |   |   |   |
           | l |   |   | b |   |   | d |
           |   |   | k |   | e |   |   |
           | j |   |   | h |   |   | f |
           |   |   |   |   |   |   |   |
           |   |   | i |   | g |   |   |

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
           |   |   | a |   | c |   |   |
           |   |   |   |   |   |   |   |
           | l |   |   | b |   |   | d |
           |   |   | k |   | e |   |   |
           | j |   |   | h |   |   | f |
           |   |   |   |   |   |   |   |
           |   |   | i |   | g |   |   |

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

       Scenario: Collinear in X
        Given the node map
            | a | b | c | d | f |
            |   |   | e |   |   |

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                           |
            | a,e       | ab,ce,ce | depart,roundabout-exit-1,arrive |
            | a,f       | ab,df,df | depart,roundabout-exit-2,arrive |

       Scenario: Collinear in Y
        Given the node map
            |   | a |
            |   | b |
            | e | c |
            |   | d |
            |   | f |

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                           |
            | a,e       | ab,ce,ce | depart,roundabout-exit-1,arrive |
            | a,f       | ab,df,df | depart,roundabout-exit-2,arrive |

       Scenario: Collinear in X,Y
        Given the node map
            | a |   |   |
            | b |   |   |
            | c | d | f |
            | e |   |   |

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                           |
            | a,e       | ab,ce,ce | depart,roundabout-exit-1,arrive |
            | a,f       | ab,df,df | depart,roundabout-exit-2,arrive |

       Scenario: Collinear in X,Y
        Given the node map
            | a |   |   |
            | d |   |   |
            | b | c | f |
            | e |   |   |

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                           |
            | a,e       | ab,ce,ce | depart,roundabout-exit-1,arrive |
            | a,f       | ab,df,df | depart,roundabout-exit-2,arrive |

       Scenario: Collinear in X,Y
        Given the node map
            | a |   |   |
            | c |   |   |
            | d | b | f |
            | e |   |   |

        And the ways
            | nodes | junction   |
            | ab    |            |
            | bcdb  | roundabout |
            | ce    |            |
            | df    |            |

        When I route I should get
            | waypoints | route    | turns                           |
            | a,e       | ab,ce,ce | depart,roundabout-exit-1,arrive |
            | a,f       | ab,df,df | depart,roundabout-exit-2,arrive |

