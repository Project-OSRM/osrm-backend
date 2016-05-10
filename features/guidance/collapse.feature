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
            | a,f       | first,third,third       | depart,new name straight,arrive            |
