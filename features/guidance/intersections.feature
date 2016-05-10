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
            | waypoints | route           | turns         | intersections                                                                |
            | a,c       | through,through | depart,arrive | true:90 true:270 false:0,true:90 true:180 false:270;true:90 true:270 false:0 |

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
            | waypoints | route           | turns         | intersections                                                              |
            | a,c       | through,through | depart,arrive | true:90 true:270 false:0,true:0 true:90 false:270;true:90 true:270 false:0 |

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
            | waypoints | route           | turns         | intersections                                                               |
            | a,c       | through,through | depart,arrive | true:90 true:270 false:0,false:0 true:90 false:270;true:90 true:270 false:0 |

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
            | waypoints | route           | turns         | intersections                                                              |
            | a,c       | through,through | depart,arrive | true:90 true:270 false:0,true:0 true:90 false:270;true:90 true:270 false:0 |

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
            | waypoints | route           | turns         | intersections                                                                                         |
            | a,d       | through,through | depart,arrive | true:90 true:270 false:0,true:0 true:90 false:270,true:90 true:180 false:270;true:90 true:270 false:0 |

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
            | waypoints | route                        | turns                          | intersections                                                                                         |
            | a,d       | through,through              | depart,arrive                  | true:90 true:270 false:0,true:0 true:90 false:270,true:90 true:180 false:270;true:90 true:270 false:0 |
            | f,a       | corner,throughbridge,through | depart,end of road left,arrive | true:0 true:180;true:90 false:180 true:270,true:0 false:90 true:270;true:90 true:270 false:0 |

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
            | waypoints | route          | turns                              | intersections                                                                                                                                        |
            | e,f       | ea,fb,fb       | depart,abcda-exit-1,arrive         | true:0 true:180;false:0 false:150 true:210,false:30 true:150 true:270;true:90 true:270 false:0                                                       |
            | e,g       | ea,gc,gc       | depart,abcda-exit-2,arrive         | true:0 true:180;false:0 false:150 true:210,false:30 true:150 true:270,true:30 true:180 false:330;true:0 true:180                                     |
            | e,h       | ea,hd,hd       | depart,abcda-exit-3,arrive         | true:0 true:180;false:0 false:150 true:210,false:30 true:150 true:270,true:30 true:180 false:330,true:90 false:210 true:330;true:90 true:270 false:0 |
            | e,2       | ea,abcda,abcda | depart,abcda-exit-undefined,arrive | true:0 true:180;false:0 false:150 true:210,false:30 true:150 true:270;true:146 false:326 false:0                                                     |
            | 1,g       | abcda,gc,gc    | depart,abcda-exit-2,arrive         | false:34 true:214 false:0;false:34 true:214,false:30 true:150 true:270,true:30 true:180 false:330;true:0 true:180                                    |
            | 1,3       | abcda,abcda    | depart,arrive                      | false:34 true:214 false:0,false:30 true:150 true:270,true:30 true:180 false:330;true:34 false:214 false:0                                            |
