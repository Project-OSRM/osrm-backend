@routing  @guidance @collapsing
Feature: Collapse

    Background:
        Given the profile "car"
        Given a grid size of 20 meters

    Scenario: Segregated Intersection, Cross Belonging to Single Street
        Given the node map
            |   |   | i | l |   |   |
            |   |   |   |   |   |   |
            | d |   | c | b |   | a |
            | e |   | f | g |   | h |
            |   |   |   |   |   |   |
            |   |   | j | k |   |   |

        And the ways
            | nodes | highway | name   | oneway |
            | ab    | primary | first  | yes    |
            | bc    | primary | first  | yes    |
            | cd    | primary | first  | yes    |
            | ef    | primary | first  | yes    |
            | fg    | primary | first  | yes    |
            | gh    | primary | first  | yes    |
            | ic    | primary | second | yes    |
            | bl    | primary | second | yes    |
            | kg    | primary | second | yes    |
            | fj    | primary | second | yes    |
            | cf    | primary | first  | yes    |
            | gb    | primary | first  | yes    |

       When I route I should get
            | waypoints | route                | turns                        |
            | a,l       | first,second,second  | depart,turn right,arrive     |
            | a,d       | first,first          | depart,arrive                |
            | a,j       | first,second,second  | depart,turn left,arrive      |
            | a,h       | first,first,first    | depart,continue uturn,arrive |
            | e,j       | first,second,second  | depart,turn right,arrive     |
            | e,h       | first,first          | depart,arrive                |
            | e,l       | first,second,second  | depart,turn left,arrive      |
            | e,d       | first,first,first    | depart,continue uturn,arrive |
            | k,h       | second,first,first   | depart,turn right,arrive     |
            | k,l       | second,second        | depart,arrive                |
            | k,d       | second,first,first   | depart,turn left,arrive      |
            | k,j       | second,second,second | depart,continue uturn,arrive |
            | i,d       | second,first,first   | depart,turn right,arrive     |
            | i,j       | second,second        | depart,arrive                |
            | i,h       | second,first,first   | depart,turn left,arrive      |
            | i,l       | second,second,second | depart,continue uturn,arrive |

    Scenario: Segregated Intersection, Cross Belonging to Correct Street
        Given the node map
            |   |   | i | l |   |   |
            |   |   |   |   |   |   |
            | d |   | c | b |   | a |
            | e |   | f | g |   | h |
            |   |   |   |   |   |   |
            |   |   | j | k |   |   |

        And the ways
            | nodes | highway | name   | oneway |
            | ab    | primary | first  | yes    |
            | bc    | primary | first  | yes    |
            | cd    | primary | first  | yes    |
            | ef    | primary | first  | yes    |
            | fg    | primary | first  | yes    |
            | gh    | primary | first  | yes    |
            | ic    | primary | second | yes    |
            | bl    | primary | second | yes    |
            | kg    | primary | second | yes    |
            | fj    | primary | second | yes    |
            | cf    | primary | second | yes    |
            | gb    | primary | second | yes    |

       When I route I should get
            | waypoints | route                | turns                        |
            | a,l       | first,second,second  | depart,turn right,arrive     |
            | a,d       | first,first          | depart,arrive                |
            | a,j       | first,second,second  | depart,turn left,arrive      |
            | a,h       | first,first,first    | depart,continue uturn,arrive |
            | e,j       | first,second,second  | depart,turn right,arrive     |
            | e,h       | first,first          | depart,arrive                |
            | e,l       | first,second,second  | depart,turn left,arrive      |
            | e,d       | first,first,first    | depart,continue uturn,arrive |
            | k,h       | second,first,first   | depart,turn right,arrive     |
            | k,l       | second,second        | depart,arrive                |
            | k,d       | second,first,first   | depart,turn left,arrive      |
            | k,j       | second,second,second | depart,continue uturn,arrive |
            | i,d       | second,first,first   | depart,turn right,arrive     |
            | i,j       | second,second        | depart,arrive                |
            | i,h       | second,first,first   | depart,turn left,arrive      |
            | i,l       | second,second,second | depart,continue uturn,arrive |

    Scenario: Segregated Intersection, Cross Belonging to Mixed Streets
        Given the node map
            |   |   | i | l |   |   |
            |   |   |   |   |   |   |
            | d |   | c | b |   | a |
            | e |   | f | g |   | h |
            |   |   |   |   |   |   |
            |   |   | j | k |   |   |

        And the ways
            | nodes | highway | name   | oneway |
            | ab    | primary | first  | yes    |
            | bc    | primary | second | yes    |
            | cd    | primary | first  | yes    |
            | ef    | primary | first  | yes    |
            | fg    | primary | first  | yes    |
            | gh    | primary | first  | yes    |
            | ic    | primary | second | yes    |
            | bl    | primary | second | yes    |
            | kg    | primary | second | yes    |
            | fj    | primary | second | yes    |
            | cf    | primary | second | yes    |
            | gb    | primary | first  | yes    |

       When I route I should get
            | waypoints | route                | turns                        |
            | a,l       | first,second,second  | depart,turn right,arrive     |
            | a,d       | first,first          | depart,arrive                |
            | a,j       | first,second,second  | depart,turn left,arrive      |
            | a,h       | first,first,first    | depart,continue uturn,arrive |
            | e,j       | first,second,second  | depart,turn right,arrive     |
            | e,h       | first,first          | depart,arrive                |
            | e,l       | first,second,second  | depart,turn left,arrive      |
            | e,d       | first,first,first    | depart,continue uturn,arrive |
            | k,h       | second,first,first   | depart,turn right,arrive     |
            | k,l       | second,second        | depart,arrive                |
            | k,d       | second,first,first   | depart,turn left,arrive      |
            | k,j       | second,second,second | depart,continue uturn,arrive |
            | i,d       | second,first,first   | depart,turn right,arrive     |
            | i,j       | second,second        | depart,arrive                |
            | i,h       | second,first,first   | depart,turn left,arrive      |
            | i,l       | second,second,second | depart,continue uturn,arrive |

    Scenario: Partly Segregated Intersection, Two Segregated Roads
        Given the node map
            |   | g |   | h |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | c |   | b |   | a |
            | d |   | e |   | f |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   | j |   | i |   |

        And the ways
            | nodes | highway | name   | oneway |
            | ab    | primary | first  | yes    |
            | bc    | primary | first  | yes    |
            | de    | primary | first  | yes    |
            | ef    | primary | first  | yes    |
            | be    | primary | first  | no     |
            | gbh   | primary | second | yes    |
            | iej   | primary | second | yes    |

       When I route I should get
            | waypoints | route                | turns                        |
            | a,h       | first,second,second  | depart,turn right,arrive     |
            | a,c       | first,first          | depart,arrive                |
            | a,j       | first,second,second  | depart,turn left,arrive      |
            | a,f       | first,first,first    | depart,continue uturn,arrive |
            | d,j       | first,second,second  | depart,turn right,arrive     |
            | d,f       | first,first          | depart,arrive                |
            | d,h       | first,second,second  | depart,turn left,arrive      |
            | d,c       | first,first,first    | depart,continue uturn,arrive |
            | g,c       | second,first,first   | depart,turn right,arrive     |
            | g,j       | second,second        | depart,arrive                |
            | g,f       | second,first,first   | depart,turn left,arrive      |
            | g,h       | second,second,second | depart,continue uturn,arrive |
            | i,f       | second,first,first   | depart,turn right,arrive     |
            | i,h       | second,second        | depart,arrive                |
            | i,c       | second,first,first   | depart,turn left,arrive      |
            | i,j       | second,second,second | depart,continue uturn,arrive |

    Scenario: Partly Segregated Intersection, Two Segregated Roads, Intersection belongs to Second
        Given the node map
            |   | g |   | h |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | c |   | b |   | a |
            | d |   | e |   | f |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   | j |   | i |   |

        And the ways
            | nodes | highway | name   | oneway |
            | ab    | primary | first  | yes    |
            | bc    | primary | first  | yes    |
            | de    | primary | first  | yes    |
            | ef    | primary | first  | yes    |
            | be    | primary | second | no     |
            | gbh   | primary | second | yes    |
            | iej   | primary | second | yes    |

       When I route I should get
            | waypoints | route                | turns                        |
            | a,h       | first,second,second  | depart,turn right,arrive     |
            | a,c       | first,first          | depart,arrive                |
            | a,j       | first,second,second  | depart,turn left,arrive      |
            | a,f       | first,first,first    | depart,continue uturn,arrive |
            | d,j       | first,second,second  | depart,turn right,arrive     |
            | d,f       | first,first          | depart,arrive                |
            | d,h       | first,second,second  | depart,turn left,arrive      |
            | d,c       | first,first,first    | depart,continue uturn,arrive |
            | g,c       | second,first,first   | depart,turn right,arrive     |
            | g,j       | second,second        | depart,arrive                |
            | g,f       | second,first,first   | depart,turn left,arrive      |
            | g,h       | second,second,second | depart,continue uturn,arrive |
            | i,f       | second,first,first   | depart,turn right,arrive     |
            | i,h       | second,second        | depart,arrive                |
            | i,c       | second,first,first   | depart,turn left,arrive      |
            | i,j       | second,second,second | depart,continue uturn,arrive |

    Scenario: Segregated Intersection, Cross Belonging to Mixed Streets - Slight Angles
        Given the node map
            |   |   | i | l |   |   |
            |   |   |   |   |   | a |
            |   |   | c | b |   | h |
            | d |   | f | g |   |   |
            | e |   |   |   |   |   |
            |   |   | j | k |   |   |

        And the ways
            | nodes | highway | name   | oneway |
            | ab    | primary | first  | yes    |
            | bc    | primary | second | yes    |
            | cd    | primary | first  | yes    |
            | ef    | primary | first  | yes    |
            | fg    | primary | first  | yes    |
            | gh    | primary | first  | yes    |
            | ic    | primary | second | yes    |
            | bl    | primary | second | yes    |
            | kg    | primary | second | yes    |
            | fj    | primary | second | yes    |
            | cf    | primary | second | yes    |
            | gb    | primary | first  | yes    |

       When I route I should get
            | waypoints | route                | turns                        |
            | a,l       | first,second,second  | depart,turn right,arrive     |
            | a,d       | first,first          | depart,arrive                |
            | a,j       | first,second,second  | depart,turn left,arrive      |
            | a,h       | first,first,first    | depart,continue uturn,arrive |
            | e,j       | first,second,second  | depart,turn right,arrive     |
            | e,h       | first,first          | depart,arrive                |
            | e,l       | first,second,second  | depart,turn left,arrive      |
            | e,d       | first,first,first    | depart,continue uturn,arrive |
            | k,h       | second,first,first   | depart,turn right,arrive     |
            | k,l       | second,second        | depart,arrive                |
            | k,d       | second,first,first   | depart,turn left,arrive      |
            | k,j       | second,second,second | depart,continue uturn,arrive |
            | i,d       | second,first,first   | depart,turn right,arrive     |
            | i,j       | second,second        | depart,arrive                |
            | i,h       | second,first,first   | depart,turn left,arrive      |
            | i,l       | second,second,second | depart,continue uturn,arrive |

    Scenario: Segregated Intersection, Cross Belonging to Mixed Streets - Slight Angles (2)
        Given the node map
            |   |   | i | l |   |   |
            |   |   |   |   |   |   |
            |   |   | c | b |   |   |
            | d |   | f | g |   | a |
            | e |   |   |   |   | h |
            |   |   | j | k |   |   |

        And the ways
            | nodes | highway | name   | oneway |
            | ab    | primary | first  | yes    |
            | bc    | primary | second | yes    |
            | cd    | primary | first  | yes    |
            | ef    | primary | first  | yes    |
            | fg    | primary | first  | yes    |
            | gh    | primary | first  | yes    |
            | ic    | primary | second | yes    |
            | bl    | primary | second | yes    |
            | kg    | primary | second | yes    |
            | fj    | primary | second | yes    |
            | cf    | primary | second | yes    |
            | gb    | primary | first  | yes    |

       When I route I should get
            | waypoints | route                | turns                        |
            | a,l       | first,second,second  | depart,turn right,arrive     |
            | a,d       | first,first          | depart,arrive                |
            | a,j       | first,second,second  | depart,turn left,arrive      |
            | a,h       | first,first,first    | depart,continue uturn,arrive |
            | e,j       | first,second,second  | depart,turn right,arrive     |
            | e,h       | first,first          | depart,arrive                |
            | e,l       | first,second,second  | depart,turn left,arrive      |
            | e,d       | first,first,first    | depart,continue uturn,arrive |
            | k,h       | second,first,first   | depart,turn right,arrive     |
            | k,l       | second,second        | depart,arrive                |
            | k,d       | second,first,first   | depart,turn left,arrive      |
            | k,j       | second,second,second | depart,continue uturn,arrive |
            | i,d       | second,first,first   | depart,turn right,arrive     |
            | i,j       | second,second        | depart,arrive                |
            | i,h       | second,first,first   | depart,turn left,arrive      |
            | i,l       | second,second,second | depart,continue uturn,arrive |

    Scenario: Entering a segregated road
        Given the node map
            |   | a | f |   |   |
            |   |   |   |   | g |
            |   | b | e |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            | c | d |   |   |   |

        And the ways
            | nodes | highway | name   | oneway |
            | abc   | primary | first  | yes    |
            | def   | primary | first  | yes    |
            | be    | primary | first  | no     |
            | ge    | primary | second | no     |

        When I route I should get
            | waypoints | route               | turns                          |
            | d,c       | first,first,first   | depart,continue uturn,arrive   |
            | a,f       | first,first,first   | depart,continue uturn,arrive   |
            | a,g       | first,second,second | depart,turn left,arrive        |
            | d,g       | first,second,second | depart,turn right,arrive       |
            | g,f       | second,first,first  | depart,turn right,arrive       |
            | g,c       | second,first,first  | depart,end of road left,arrive |


    Scenario: Do not collapse turning roads
        Given the node map
            |   |   | e |   |   |
            |   |   | c |   | d |
            | a |   | b | f |   |

        And the ways
            | nodes | highway | name   |
            | ab    | primary | first  |
            | bc    | primary | first  |
            | cd    | primary | first  |
            | ce    | primary | second |
            | bf    | primary | third  |

        When I route I should get
            | waypoints | route                   | turns                                      |
            | a,d       | first,first,first,first | depart,continue left,continue right,arrive |
            | a,e       | first,second,second     | depart,turn left,arrive                    |
            | a,f       | first,third,third       | depart,turn straight,arrive                |

     Scenario: Bridge on unnamed road
        Given the node map
            | a | b |   |   |   | c | d |

        And the ways
            | nodes | highway | name   |
            | ab    | primary |        |
            | bc    | primary | Bridge |
            | cd    | primary |        |

        When I route I should get
            | waypoints | route | turns         |
            | a,d       | ,     | depart,arrive |

     Scenario: Crossing Bridge into Segregated Turn
        Given the node map
            |   |   |   |   |   | f |
            | i | h |   |   | g | e |
            | a | b |   |   | c | d |

        And the ways
            | nodes | highway | oneway | name        |
            | ab    | primary | yes    | to_bridge   |
            | bc    | primary | yes    | bridge      |
            | cd    | primary | yes    | off_bridge  |
            | de    | primary | yes    |             |
            | ef    | primary | no     | target_road |
            | eg    | primary | yes    | off_bridge  |
            | gh    | primary | yes    | bridge      |
            | hi    | primary | yes    | to_bridge   |

        When I route I should get
            | waypoints | route                             | turns                   |
            | a,f       | to_bridge,target_road,target_road | depart,turn left,arrive |

    Scenario: Pankenbruecke
        Given the node map
            | h |   |   |   |   |   | i |   |   |   |   |   |   |
            |   |   | b | c | d | e | f |   |   |   |   |   | g |
            | a |   |   |   |   |   |   |   |   |   |   |   |   |

        And the ways
            | nodes | highway | name    | oneway |
            | abh   | primary | inroad  | yes    |
            | bc    | primary | inroad  | no     |
            | cd    | primary | bridge  | no     |
            | defg  | primary | outroad | no     |
            | fi    | primary | cross   | no     |

       When I route I should get
            | waypoints | route                  | turns                           |
            | a,g       | inroad,outroad,outroad | depart,new name straight,arrive |
            | a,i       | inroad,cross,cross     | depart,turn left,arrive         |

     Scenario: Close Turns - Don't Collapse
        Given the node map
            |   | g | d |   |
            |   |   |   |   |
            | e | b | c | f |
            |   |   |   |   |
            |   | a | h |   |

        And the ways
            | nodes | highway | name     |
            | ab    | primary | in       |
            | ebcf  | primary | cross    |
            | cd    | primary | out      |
            | bg    | primary | straight |
            | ch    | primary | reverse  |

        When I route I should get
            | waypoints | route                    | turns                               |
            | a,d       | in,cross,out,out         | depart,turn right,turn left,arrive  |
            | a,h       | in,cross,reverse,reverse | depart,turn right,turn right,arrive |
            | g,d       | straight,cross,out,out   | depart,turn left,turn left,arrive   |

     Scenario: No Name During Turns
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway  | name |
            | ab    | tertiary | road |
            | bc    | tertiary |      |
            | cd    | tertiary | road |

        When I route I should get
            | waypoints | route     | turns         |
            | a,d       | road,road | depart,arrive |

    Scenario: No Name During Turns, Random Oneway
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway  | name | oneway |
            | ab    | tertiary | road | no     |
            | bc    | tertiary |      | yes    |
            | cd    | tertiary | road | no     |

        When I route I should get
            | waypoints | route     | turns         |
            | a,d       | road,road | depart,arrive |

    Scenario: Pulled Back Turn
        Given the node map
            |   |   | d |
            | a | b | c |
            |   | e |   |

        And the ways
            | nodes | highway  | name  |
            | abc   | tertiary | road  |
            | cd    | tertiary | left  |
            | be    | tertiary | right |

        When I route I should get
            | waypoints | route            | turns                    |
            | a,d       | road,left,left   | depart,turn left,arrive  |
            | a,e       | road,right,right | depart,turn right,arrive |

    Scenario: No Name During Turns, keep important turns
        Given the node map
            | a | b | e |
            |   | c | d |

        And the ways
            | nodes | highway  | name  |
            | ab    | tertiary | road  |
            | bc    | tertiary |       |
            | cd    | tertiary | road  |
            | be    | tertiary | other |

        When I route I should get
            | waypoints | route          | turns                        |
            | a,d       | road,road,road | depart,continue right,arrive |

    Scenario: Segregated Intersection into Slight Turn
        Given the node map
            | h |   |   |   |   |   |   |
            | a |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   | g |   |   |   |   |
            |   |   | b | f |   |   |   |
            |   |   |   | c |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | e |
            |   |   |   |   |   |   | d |
            |   |   | j | i |   |   |   |

        And the ways
            | nodes | highway   | name | oneway |
            | abcd  | primary   | road | yes    |
            | efgh  | primary   | road | yes    |
            | icf   | secondary | in   | yes    |
            | gbj   | secondary | out  | yes    |

        When I route I should get
            | waypoints | route        | turns                           |
            | i,h       | in,road,road | depart,turn left,arrive         |
            | a,d       | road,road    | depart,arrive                   |
            | a,j       | road,out,out | depart,turn slight right,arrive |

    Scenario: Segregated Intersection into Very Slight Turn
        Given the node map
            | h |   |   |   |   |   |   |
            | a |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   | g |   |   |   |   |
            |   |   | b |   |   |   |   |
            |   |   |   | f |   |   |   |
            |   |   |   | c |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | e |
            |   |   |   |   |   |   | d |
            |   |   | j | i |   |   |   |

        And the ways
            | nodes | highway   | name | oneway |
            | abcd  | primary   | road | yes    |
            | efgh  | primary   | road | yes    |
            | icf   | secondary | in   | yes    |
            | gbj   | secondary | out  | yes    |

        When I route I should get
            | waypoints | route        | turns                           |
            | i,h       | in,road,road | depart,turn slight left,arrive  |
            | a,d       | road,road    | depart,arrive                   |
            | a,j       | road,out,out | depart,turn slight right,arrive |

    Scenario: Don't collapse everything to u-turn / too wide
        Given the node map
            | a |   | b |   | e |
            |   |   |   |   |   |
            | d |   | c |   | f |

        And the ways
            | nodes | highway   | name   |
            | abcd  | primary   | road   |
            | be    | secondary | top    |
            | cf    | secondary | bottom |

        When I route I should get
            | waypoints | turns                                   | route               |
            | a,d       | depart,continue right,turn right,arrive | road,road,road,road |
            | d,a       | depart,continue left,turn left,arrive   | road,road,road,road |

    Scenario: Forking before a turn
        Given the node map
            |   |   |   | g |   |
            |   |   |   |   |   |
            |   |   |   | c |   |
            | a |   | b | d | e |
            |   |   |   |   |   |
            |   |   |   | f |   |

        And the ways
            | nodes | name  | oneway | highway   |
            | ab    | road  | yes    | primary   |
            | bd    | road  | yes    | primary   |
            | bc    | road  | yes    | primary   |
            | de    | road  | yes    | primary   |
            | fdcg  | cross | no     | secondary |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | fdcg   | d        | no_left_turn  |
            | restriction | bc       | fdcg   | c        | no_right_turn |

        When I route I should get
          | waypoints | route            | turns                   |
          | a,g       | road,cross,cross | depart,turn left,arrive |
          | a,e       | road,road        | depart,arrive           |

    Scenario: Forking before a turn (narrow)
        Given the node map
            |   |   |   | g |   |
            |   |   |   |   |   |
            |   |   |   | c |   |
            | a | b |   | d | e |
            |   |   |   |   |   |
            |   |   |   | f |   |

        And the ways
            | nodes | name  | oneway | highway   |
            | ab    | road  | yes    | primary   |
            | bd    | road  | yes    | primary   |
            | bc    | road  | yes    | primary   |
            | de    | road  | yes    | primary   |
            | fdcg  | cross | no     | secondary |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | fdcg   | d        | no_left_turn  |
            | restriction | bc       | fdcg   | c        | no_right_turn |

        When I route I should get
          | waypoints | route            | turns                   |
          | a,g       | road,cross,cross | depart,turn left,arrive |
          | a,e       | road,road        | depart,arrive           |

    Scenario: Forking before a turn (forky)
        Given the node map
            |   |   |   | g |   |   |
            |   |   |   |   |   |   |
            |   |   |   | c |   |   |
            | a | b |   |   |   |   |
            |   |   |   |   | d |   |
            |   |   |   |   | f | e |

        And the ways
            | nodes | name  | oneway | highway   |
            | ab    | road  | yes    | primary   |
            | bd    | road  | yes    | primary   |
            | bc    | road  | yes    | primary   |
            | de    | road  | yes    | primary   |
            | fdcg  | cross | no     | secondary |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | fdcg   | d        | no_left_turn  |
            | restriction | bc       | fdcg   | c        | no_right_turn |

        When I route I should get
            | waypoints | route                 | turns                                          |
            | a,g       | road,cross,cross      | depart,turn left,arrive                        |
            | a,e       | road,road,road        | depart,continue slight right,arrive            |
            # We should discuss whether the next item should be collapsed to depart,turn right,arrive.
            | a,f       | road,road,cross,cross | depart,continue slight right,turn right,arrive |

    Scenario: On-Off on Highway
        Given the node map
            | f |   |   |   |
            | a | b | c | d |
            |   |   |   | e |

        And the ways
            | nodes | name | highway       | oneway |
            | abcd  | Hwy  | motorway      | yes    |
            | fb    | on   | motorway_link | yes    |
            | ce    | off  | motorway_link | yes    |

        When I route I should get
            | waypoints | route          | turns                                           |
            | a,d       | Hwy,Hwy        | depart,arrive                                   |
            | f,d       | on,Hwy,Hwy     | depart,merge slight right,arrive                |
            | f,e       | on,Hwy,off,off | depart,merge slight right,off ramp right,arrive |
            | a,e       | Hwy,off,off    | depart,off ramp right,arrive                    |

    @negative @straight
    Scenario: Don't collapse going straight if actual turn
        Given the node map
            |   | c | e |   |   |
            |   |   | d |   | f |
            |   |   |   |   |   |
            |   |   | b |   |   |
            |   |   |   |   |   |
            |   |   |   |   |   |
            |   |   | a |   |   |

        And the ways
            | nodes | name     | highway     |
            | abc   | main     | primary     |
            | bde   | straight | residential |
            | df    | right    | residential |

        When I route I should get
            | waypoints | route                     | turns                                  |
            | a,c       | main,main                 | depart,arrive                          |
            | a,e       | main,straight,straight    | depart,turn straight,arrive            |
            | a,f       | main,straight,right,right | depart,turn straight,turn right,arrive |
