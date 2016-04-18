@routing  @guidance
Feature: Simple Turns

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Four Way Intersection
        Given the node map
            |   | c |   |
            | a | b | e |
            |   | d |   |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | db     | primary |
            | eb     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,cb,cb | depart,turn left,arrive         |
            | a,e       | ab,eb,eb | depart,new name straight,arrive |
            | a,d       | ab,db,db | depart,turn right,arrive        |
            | c,a       | cb,ab,ab | depart,turn right,arrive        |
            | c,d       | cb,db,db | depart,new name straight,arrive |
            | c,e       | cb,eb,eb | depart,turn left,arrive         |
            | d,a       | db,ab,ab | depart,turn left,arrive         |
            | d,c       | db,cb,cb | depart,new name straight,arrive |
            | d,e       | db,eb,eb | depart,turn right,arrive        |
            | e,a       | eb,ab,ab | depart,new name straight,arrive |
            | e,c       | eb,cb,cb | depart,turn right,arrive        |
            | e,d       | eb,db,db | depart,turn left,arrive         |

    Scenario: Rotated Four Way Intersection
        Given the node map
            | a |   | c |
            |   | b |   |
            | d |   | e |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | db     | primary |
            | eb     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,cb,cb | depart,turn left,arrive         |
            | a,e       | ab,eb,eb | depart,new name straight,arrive |
            | a,d       | ab,db,db | depart,turn right,arrive        |
            | c,a       | cb,ab,ab | depart,turn right,arrive        |
            | c,d       | cb,db,db | depart,new name straight,arrive |
            | c,e       | cb,eb,eb | depart,turn left,arrive         |
            | d,a       | db,ab,ab | depart,turn left,arrive         |
            | d,c       | db,cb,cb | depart,new name straight,arrive |
            | d,e       | db,eb,eb | depart,turn right,arrive        |
            | e,a       | eb,ab,ab | depart,new name straight,arrive |
            | e,c       | eb,cb,cb | depart,turn right,arrive        |
            | e,d       | eb,db,db | depart,turn left,arrive         |


    Scenario: Four Way Intersection Through Street
        Given the node map
            |   | c |   |
            | a | b | e |
            |   | d |   |

        And the ways
            | nodes  | highway |
            | abe    | primary |
            | cb     | primary |
            | db     | primary |

       When I route I should get
            | waypoints | route      | turns                           |
            | a,c       | abe,cb,cb  | depart,turn left,arrive         |
            | a,e       | abe,abe    | depart,arrive                   |
            | a,d       | abe,db,db  | depart,turn right,arrive        |
            | c,a       | cb,abe,abe | depart,turn right,arrive        |
            | c,d       | cb,db,db   | depart,new name straight,arrive |
            | c,e       | cb,abe,abe | depart,turn left,arrive         |
            | d,a       | db,abe,abe | depart,turn left,arrive         |
            | d,c       | db,cb,cb   | depart,new name straight,arrive |
            | d,e       | db,abe,abe | depart,turn right,arrive        |
            | e,a       | abe,abe    | depart,arrive                   |
            | e,c       | abe,cb,cb  | depart,turn right,arrive        |
            | e,d       | abe,db,db  | depart,turn left,arrive         |

    Scenario: Four Way Intersection Double Through Street
        Given the node map
            |   | c |   |
            | a | b | e |
            |   | d |   |

        And the ways
            | nodes  | highway |
            | abe    | primary |
            | cbd    | primary |

       When I route I should get
            | waypoints | route       | turns                    |
            | a,c       | abe,cbd,cbd | depart,turn left,arrive  |
            | a,e       | abe,abe     | depart,arrive            |
            | a,d       | abe,cbd,cbd | depart,turn right,arrive |
            | c,a       | cbd,abe,abe | depart,turn right,arrive |
            | c,d       | cbd,cbd     | depart,arrive            |
            | c,e       | cbd,abe,abe | depart,turn left,arrive  |
            | d,a       | cbd,abe,abe | depart,turn left,arrive  |
            | d,c       | cbd,cbd     | depart,arrive            |
            | d,e       | cbd,abe,abe | depart,turn right,arrive |
            | e,a       | abe,abe     | depart,arrive            |
            | e,c       | abe,cbd,cbd | depart,turn right,arrive |
            | e,d       | abe,cbd,cbd | depart,turn left,arrive  |

    Scenario: Three Way Intersection
        Given the node map
            |   | c |   |
            | a | b | d |

        And the ways
            | nodes  | highway |
            | ab     | primary |
            | cb     | primary |
            | db     | primary |

       When I route I should get
            | waypoints | route    | turns                           |
            | a,c       | ab,cb,cb | depart,turn left,arrive         |
            | a,d       | ab,db,db | depart,new name straight,arrive |
            | d,c       | db,cb,cb | depart,turn right,arrive        |
            | d,a       | db,ab,ab | depart,new name straight,arrive |

    Scenario: Three Way Intersection on Through Street
        Given the node map
            |   | d |   |
            | a | b | c |

        And the ways
            | nodes  | highway |
            | abc    | primary |
            | db     | primary |

       When I route I should get
            | waypoints | route     | turns                    |
            | a,c       | abc,abc   | depart,arrive            |
            | a,d       | abc,db,db | depart,turn left,arrive  |
            | c,a       | abc,abc   | depart,arrive            |
            | c,d       | abc,db,db | depart,turn right,arrive |

     Scenario: High Degree Intersection
        Given the node map
            | i |   | b |   | c |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | h |   | a |   | d |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | g |   | f |   | e |

        And the ways
            | nodes | highway |
            | ab    | primary |
            | ac    | primary |
            | ad    | primary |
            | ae    | primary |
            | af    | primary |
            | ag    | primary |
            | ah    | primary |
            | ai    | primary |

        When I route I should get
            | waypoints | route    | turns                           |
            | b,c       | ab,ac,ac | depart,turn sharp left,arrive   |
            | b,d       | ab,ad,ad | depart,turn left,arrive         |
            | b,e       | ab,ae,ae | depart,turn slight left,arrive  |
            | b,f       | ab,af,af | depart,new name straight,arrive |
            | b,g       | ab,ag,ag | depart,turn slight right,arrive |
            | b,h       | ab,ah,ah | depart,turn right,arrive        |
            | b,i       | ab,ai,ai | depart,turn sharp right,arrive  |

    Scenario: Disturbed High Degree Intersection
        Given the node map
            |   |   | b |   |   |
            | i |   |   |   | c |
            |   |   |   |   |   |
            | h |   | a |   | d |
            |   |   |   |   |   |
            | g |   |   |   | e |
            |   |   | f |   |   |

        And the ways
            | nodes | highway |
            | ab    | primary |
            | ac    | primary |
            | ad    | primary |
            | ae    | primary |
            | af    | primary |
            | ag    | primary |
            | ah    | primary |
            | ai    | primary |

        When I route I should get
            | waypoints | route    | turns                           |
            | b,c       | ab,ac,ac | depart,turn sharp left,arrive   |
            | b,d       | ab,ad,ad | depart,turn left,arrive         |
            | b,e       | ab,ae,ae | depart,turn slight left,arrive  |
            | b,f       | ab,af,af | depart,new name straight,arrive |
            | b,g       | ab,ag,ag | depart,turn slight right,arrive |
            | b,h       | ab,ah,ah | depart,turn right,arrive        |
            | b,i       | ab,ai,ai | depart,turn sharp right,arrive  |

    Scenario: Turn instructions at high latitude
        Given the node locations
            | node | lat       | lon      |
            | a    | 55.68740  | 12.52430 |
            | b    | 55.68745  | 12.52409 |
            | c    | 55.68711  | 12.52383 |
            | d    | 55.68745  | 12.52450 |
            | e    | 55.68755  | 12.52450 |
            | x    | -55.68740 | 12.52430 |
            | y    | -55.68745 | 12.52409 |
            | z    | -55.68711 | 12.52383 |
            | v    | -55.68745 | 12.52450 |
            | w    | -55.68755 | 12.52450 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | bd    |
            | be    |
            | xy    |
            | yz    |
            | vy    |
            | wy    |

        When I route I should get
            | from | to | route    | turns                    |
            | a    | c  | ab,bc,bc | depart,turn left,arrive  |
            | c    | a  | bc,ab,ab | depart,turn right,arrive |
            | x    | z  | xy,yz,yz | depart,turn right,arrive |
            | z    | x  | yz,xy,xy | depart,turn left,arrive  |

    Scenario: Four Way Intersection Double Through Street Segregated
        Given the node map
            |   | b |   | c |   |
            | i |   |   |   | d |
            |   |   | a |   |   |
            | h |   |   |   | e |
            |   | g |   | f |   |

        And the ways
            | nodes  | highway | oneway | name   |
            | ha     | primary | yes    | first  |
            | ai     | primary | yes    | first  |
            | ae     | primary | yes    | first  |
            | da     | primary | yes    | first  |
            | ba     | primary | yes    | second |
            | ac     | primary | yes    | second |
            | fa     | primary | yes    | second |
            | ag     | primary | yes    | second |

       When I route I should get
            | waypoints | route                | turns                        |
            | f,e       | second,first,first   | depart,turn right,arrive     |
            | f,c       | second,second        | depart,arrive                |
            | f,i       | second,first,first   | depart,turn left,arrive      |
            | f,g       | second,second,second | depart,continue uturn,arrive |
            | d,c       | first,second,second  | depart,turn right,arrive     |
            | d,i       | first,first          | depart,arrive                |
            | d,g       | first,second,second  | depart,turn left,arrive      |
            | d,e       | first,first,first    | depart,continue uturn,arrive |
            | b,i       | second,first,first   | depart,turn right,arrive     |
            | b,g       | second,second        | depart,arrive                |
            | b,e       | second,first,first   | depart,turn left,arrive      |
            | b,c       | second,second,second | depart,continue uturn,arrive |
            | h,g       | first,second,second  | depart,turn right,arrive     |
            | h,e       | first,first          | depart,arrive                |
            | h,c       | first,second,second  | depart,turn left,arrive      |
            | h,i       | first,first,first    | depart,continue uturn,arrive |

    Scenario: Three Way Similar Sharp Turns
        Given the node map
            | a |   |   |   | b |
            | c |   |   |   |   |
            |   | d |   |   |   |

        And the ways
            | nodes | highway |
            | ab    | primary |
            | bc    | primary |
            | bd    | primary |

        When I route I should get
            | waypoints | route    | turns                          |
            | a,c       | ab,bc,bc | depart,turn sharp right,arrive |
            | a,d       | ab,bd,bd | depart,turn sharp right,arrive |
            | d,c       | bd,bc,bc | depart,turn sharp left,arrive  |
            | d,a       | bd,ab,ab | depart,turn sharp left,arrive  |

    Scenario: Left Turn Assignment (1)
        Given the node map
            |   |   |   |   | d |
            | a |   | b |   | c |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,turn slight left,arrive |

    Scenario: Left Turn Assignment (2)
        Given the node map
            |   |   |   |   | d |
            |   |   |   |   |   |
            | a |   | b |   | c |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   |
            | a,d       | abc,bd,bd | depart,turn left,arrive |

    Scenario: Left Turn Assignment (3)
        Given the node map
            |   |   |   | d |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | a |   | b |   | c |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   |
            | a,d       | abc,bd,bd | depart,turn left,arrive |

    Scenario: Left Turn Assignment (4)
        Given the node map
            |   |   | d |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | a |   | b |   | c |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   |
            | a,d       | abc,bd,bd | depart,turn left,arrive |

    Scenario: Left Turn Assignment (5)
        Given the node map
            |   | d |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | a |   | b |   | c |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                   |
            | a,d       | abc,bd,bd | depart,turn left,arrive |

    Scenario: Left Turn Assignment (6)
        Given the node map
            | d |   |   |   |   |
            |   |   |   |   |   |
            | a |   | b |   | c |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                         |
            | a,d       | abc,bd,bd | depart,turn sharp left,arrive |

    Scenario: Left Turn Assignment (7)
        Given the node map
            | d |   |   |   |   |
            | a |   | b |   | c |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                         |
            | a,d       | abc,bd,bd | depart,turn sharp left,arrive |

    Scenario: Right Turn Assignment (1)
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   |   |   | d |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                           |
            | a,d       | abc,bd,bd | depart,turn slight right,arrive |

    Scenario: Right Turn Assignment (2)
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   |   |   | d |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    |
            | a,d       | abc,bd,bd | depart,turn right,arrive |

    Scenario: Right Turn Assignment (3)
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   | d |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    |
            | a,d       | abc,bd,bd | depart,turn right,arrive |

    Scenario: Right Turn Assignment (4)
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   | d |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    |
            | a,d       | abc,bd,bd | depart,turn right,arrive |

    Scenario: Right Turn Assignment (5)
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   | d |   |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                    |
            | a,d       | abc,bd,bd | depart,turn right,arrive |

    Scenario: Right Turn Assignment (6)
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            |   |   |   |   |   |
            | d |   |   |   |   |


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive |

    Scenario: Right Turn Assignment (7)
        Given the node map
            |   |   | e |   |   |
            | a |   | b |   | c |
            | d |   |   |   |   |


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |

        When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive |

   Scenario: Right Turn Assignment Two Turns
        Given the node map
            |   |   | f |   |   |
            | a |   | b |   | c |
            |   |   |   |   |   |
            | d | e |   |   |   |


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                          |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive |
            | a,e       | abc,be,be | depart,turn right,arrive       |

   Scenario: Right Turn Assignment Two Turns (2)
        Given the node map
            |   |   | f |   | c |
            | a |   | b |   |   |
            |   |   |   |   | e |
            |   |   |   | d |   |


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           |
            | a,d       | abc,bd,bd | depart,turn right,arrive        |
            | a,e       | abc,be,be | depart,turn slight right,arrive |

   Scenario: Right Turn Assignment Two Turns (3)
        Given the node map
            |   |   | f |   |   |
            | a |   | b |   | c |
            |   |   |   |   | e |
            |   |   |   | d |   |


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           |
            | a,d       | abc,bd,bd | depart,turn right,arrive        |
            | a,e       | abc,be,be | depart,turn slight right,arrive |

   Scenario: Right Turn Assignment Two Turns (4)
        Given the node map
            |   |   | f |   |   |
            | a |   | b |   | c |
            |   |   |   |   |   |
            |   |   | d |   | e |


        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |

        When I route I should get
            | waypoints | route     | turns                           |
            | a,d       | abc,bd,bd | depart,turn right,arrive        |
            | a,e       | abc,be,be | depart,turn slight right,arrive |

   Scenario: Right Turn Assignment Three Turns
        Given the node map
            |   |   | g |   |   |
            | a |   | b |   | c |
            |   | d |   | f |   |
            |   |   | e |   |   |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | bd    | primary |
            | be    | primary |
            | bf    | primary |
            | bg    | primary |

        When I route I should get
            | waypoints | route     | turns                           |
            | a,d       | abc,bd,bd | depart,turn sharp right,arrive  |
            | a,e       | abc,be,be | depart,turn right,arrive        |
            | a,f       | abc,bf,bf | depart,turn slight right,arrive |

    Scenario: Slight Turn involving Oneways
        Given the node map
            |   |   | a |   |   |
            |   |   |   |   |   |
            |   |   | b |   | e |
            | d |   |   |   |   |
            |   |   | c |   |   |

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | yes    |
            | dbe   | primary | no     |

        When I route I should get
            | waypoints | route   | turns         |
            | a,c       | abc,abc | depart,arrive |
            | d,e       | dbe,dbe | depart,arrive |
            | e,d       | dbe,dbe | depart,arrive |

    Scenario: Slight Turn involving Oneways
        Given the node map
            |   |   |   | a |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   | b |   | e |
            | d |   |   |   |   |
            |   |   | c |   |   |

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | yes    |
            | dbe   | primary | no     |

        When I route I should get
            | waypoints | route   | turns         |
            | a,c       | abc,abc | depart,arrive |
            | d,e       | dbe,dbe | depart,arrive |
            | e,d       | dbe,dbe | depart,arrive |


    Scenario: Slight Turn involving Oneways - Name Change
        Given the node map
            |   |   | a |   |   |
            |   |   |   |   |   |
            |   |   | b |   | e |
            | d |   |   |   |   |
            |   |   | c |   |   |

        And the ways
            | nodes | highway | oneway |
            | abc   | primary | yes    |
            | db    | primary | no     |
            | be    | primary | no     |

        When I route I should get
            | waypoints | route    | turns                               |
            | a,c       | abc,abc  | depart,arrive                       |
            | d,e       | db,be,be | depart,new name slight right,arrive |
            | e,d       | be,db,db | depart,new name slight left,arrive  |
