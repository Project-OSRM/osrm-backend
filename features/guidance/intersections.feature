@routing  @guidance @intersections
Feature: Intersections Data

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Passing Three Way South
        Given the node map
            | a |   | b |   | c |
            |   |   | d |   |   |

        And the ways
            | nodes  | name    |
            | ab     | through |
            | bc     | through |
            | bd     | corner  |

       When I route I should get
            | waypoints | route           | turns         | intersections                               |
            | a,c       | through,through | depart,arrive | true:90,true:90 true:180 false:270;true:270 |

    Scenario: Passing Three Way North
        Given the node map
            |   |   | d |   |   |
            | a |   | b |   | c |

        And the ways
            | nodes  | name    |
            | ab     | through |
            | bc     | through |
            | bd     | corner  |

       When I route I should get
            | waypoints | route           | turns         | intersections                             |
            | a,c       | through,through | depart,arrive | true:90,true:0 true:90 false:270;true:270 |

    Scenario: Passing Oneway Street In
        Given the node map
            |   |   | d |   |   |
            | a |   | b |   | c |

        And the ways
            | nodes  | name    | oneway |
            | ab     | through | no     |
            | bc     | through | no     |
            | db     | corner  | yes    |

       When I route I should get
            | waypoints | route           | turns         | intersections                              |
            | a,c       | through,through | depart,arrive | true:90,false:0 true:90 false:270;true:270 |

    Scenario: Passing Oneway Street Out
        Given the node map
            |   |   | d |   |   |
            | a |   | b |   | c |

        And the ways
            | nodes  | name    | oneway |
            | ab     | through | no     |
            | bc     | through | no     |
            | bd     | corner  | yes    |

       When I route I should get
            | waypoints | route           | turns         | intersections                             |
            | a,c       | through,through | depart,arrive | true:90,true:0 true:90 false:270;true:270 |

    Scenario: Passing Two Intersections
        Given the node map
            |   |   | e |   |   |   |   |
            | a |   | b |   | c |   | d |
            |   |   |   |   | f |   |   |

        And the ways
            | nodes  | name    |
            | ab     | through |
            | bc     | through |
            | cd     | through |
            | be     | corner  |
            | cf     | corner  |

       When I route I should get
            | waypoints | route           | turns         | intersections                                                        |
            | a,d       | through,through | depart,arrive | true:90,true:0 true:90 false:270,true:90 true:180 false:270;true:270 |

    Scenario: Regression test #2424
        Given the node map
            |   |   | e |   |   |   |   |   | i |   |   |   |   |
            | a |   | b |   | c |   | d |   | h |   | k |   | m |
            |   |   |   |   | f |   |   |   |   |   | l |   |   |

        And the ways
            | nodes  | name    |
            | abcd   | Fritz-Elsas-Straße |
            | hkm    | Fritz-Elsas-Straße |
            | dhi    | Martin-Luther-Straße |
            | be     | corner  |
            | kl     | corner  |
            | cf     | corner  |

       When I route I should get
            | waypoints | route | turns         |
            | a,m       | Fritz-Elsas-Straße,Fritz-Elsas-Straße| depart,arrive |

    Scenario: Passing Two Intersections, Collapsing
        Given the node map
            |   |   | e |   |   |   |   |
            | a |   | b |   | c |   | d |
            |   |   |   |   | f |   |   |

        And the ways
            | nodes  | name          |
            | ab     | through       |
            | bc     | throughbridge |
            | cd     | through       |
            | be     | corner        |
            | cf     | corner        |

       When I route I should get
            | waypoints | route                  | turns                          | intersections                                                        |
            | a,d       | through,through        | depart,arrive                  | true:90,true:0 true:90 false:270,true:90 true:180 false:270;true:270 |
            | f,a       | corner,through,through | depart,end of road left,arrive | true:0;true:90 false:180 true:270,true:0 false:90 true:270;true:90   |

    Scenario: Roundabouts
        Given the node map
            |   |   |   |   | e |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   | a |   |   |   |   |
            |   |   |   | 1 |   | 4 |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            | f |   | b |   |   |   | d |   | h |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   | 2 |   | 3 |   |   |   |
            |   |   |   |   | c |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   | g |   |   |   |   |

        And the ways
            | nodes | junction   |
            | abcda | roundabout |
            | ea    |            |
            | fb    |            |
            | gc    |            |
            | hd    |            |

        When I route I should get
            | waypoints | route          | turns                              | intersections                                                         |
            | e,f       | ea,fb,fb       | depart,abcda-exit-1,arrive         | true:180;false:0 false:150 true:210,false:30 true:150 true:270;true:90 |
            | e,g       | ea,gc,gc       | depart,abcda-exit-2,arrive         | true:180;false:0 false:150 true:210,false:30 true:150 true:270,true:30 true:180 false:330;true:0|
            | e,h       | ea,hd,hd       | depart,abcda-exit-3,arrive         | true:180;false:0 false:150 true:210,false:30 true:150 true:270,true:30 true:180 false:330,true:90 false:210 true:330;true:270 |
            | e,2       | ea,abcda,abcda | depart,abcda-exit-undefined,arrive | true:180;false:0 false:150 true:210,false:30 true:150 true:270;true:327 +-1|
            | 1,g       | abcda,gc,gc    | depart,abcda-exit-2,arrive         | true:214;true:214,false:30 true:150 true:270,true:30 true:180 false:330;true:0|
            | 1,3       | abcda,abcda    | depart,arrive                      | true:214,false:30 true:150 true:270,true:30 true:180 false:330;true:214|
