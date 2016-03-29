@routing  @guidance
Feature: Basic Roundabout

    Background:
        Given the profile "testbot"
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
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                           |
           | a,d       | ab,cd,cd | depart,roundabout-exit-1,arrive |
           | a,f       | ab,ef,ef | depart,roundabout-exit-2,arrive |
           | a,h       | ab,gh,gh | depart,roundabout-exit-3,arrive |
           | d,f       | cd,ef,ef | depart,roundabout-exit-1,arrive |
           | d,h       | cd,gh,gh | depart,roundabout-exit-2,arrive |
           | d,a       | cd,ab,ab | depart,roundabout-exit-3,arrive |
           | f,h       | ef,gh,gh | depart,roundabout-exit-1,arrive |
           | f,a       | ef,ab,ab | depart,roundabout-exit-2,arrive |
           | f,d       | ef,cd,cd | depart,roundabout-exit-3,arrive |
           | h,a       | gh,ab,ab | depart,roundabout-exit-1,arrive |
           | h,d       | gh,cd,cd | depart,roundabout-exit-2,arrive |
           | h,f       | gh,ef,ef | depart,roundabout-exit-3,arrive |

    Scenario: Only Enter
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
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route    | turns                          |
           | a,b       | ab,ab    | depart,arrive                  |
           | a,c       | ab,bcegb | depart,roundabout-enter,arrive |
           | a,e       | ab,bcegb | depart,roundabout-enter,arrive |
           | a,g       | ab,bcegb | depart,roundabout-enter,arrive |
           | d,c       | cd,cd    | depart,arrive                  |
           | d,e       | cd,bcegb | depart,roundabout-enter,arrive |
           | d,g       | cd,bcegb | depart,roundabout-enter,arrive |
           | d,b       | cd,bcegb | depart,roundabout-enter,arrive |
           | f,e       | ef,ef    | depart,arrive                  |
           | f,g       | ef,bcegb | depart,roundabout-enter,arrive |
           | f,b       | ef,bcegb | depart,roundabout-enter,arrive |
           | f,c       | ef,bcegb | depart,roundabout-enter,arrive |
           | h,g       | gh,gh    | depart,arrive                  |
           | h,b       | gh,bcegb | depart,roundabout-enter,arrive |
           | h,c       | gh,bcegb | depart,roundabout-enter,arrive |
           | h,e       | gh,bcegb | depart,roundabout-enter,arrive |

    Scenario: Only Exit
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
            | bcegb  | roundabout |

       When I route I should get
           | waypoints | route       | turns                           |
           | b,a       | ab,ab       | depart,arrive                   |
           | b,d       | bcegb,cd,cd | depart,roundabout-exit-1,arrive |
           | b,f       | bcegb,ef,ef | depart,roundabout-exit-2,arrive |
           | b,h       | bcegb,gh,gh | depart,roundabout-exit-3,arrive |
           | c,d       | cd,cd       | depart,arrive                   |
           | c,f       | bcegb,ef,ef | depart,roundabout-exit-1,arrive |
           | c,h       | bcegb,gh,gh | depart,roundabout-exit-2,arrive |
           | c,a       | bcegb,ab,ab | depart,roundabout-exit-3,arrive |
           | e,f       | ef,ef       | depart,arrive                   |
           | e,h       | bcegb,gh,gh | depart,roundabout-exit-1,arrive |
           | e,a       | bcegb,ab,ab | depart,roundabout-exit-2,arrive |
           | e,d       | bcegb,cd,cd | depart,roundabout-exit-3,arrive |
           | g,h       | gh,gh       | depart,arrive                   |
           | g,a       | bcegb,ab,ab | depart,roundabout-exit-1,arrive |
           | g,d       | bcegb,cd,cd | depart,roundabout-exit-2,arrive |
           | g,f       | bcegb,ef,ef | depart,roundabout-exit-3,arrive |

    Scenario: Drive Around
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
           |   | a |   | c |   |
           | l |   | b |   | d |
           |   | k |   | e |   |
           | j |   | h |   | f |
           |   | i |   | g |   |

        And the ways
           | nodes | junction   | oneway |
           | abc   |            | yes    |
           | def   |            | yes    |
           | ghi   |            | yes    |
           | jkl   |            | yes    |
           | behkb | roundabout | yes    |

        When I route I should get
           | waypoints | route       | turns                           |
           | a,c       | abc,abc,abc | depart,roundabout-exit-1,arrive |
           | a,f       | abc,def,def | depart,roundabout-exit-2,arrive |
           | a,i       | abc,ghi,ghi | depart,roundabout-exit-3,arrive |
           | a,l       | abc,jkl,jkl | depart,roundabout-exit-4,arrive |
           | d,f       | def,def,def | depart,roundabout-exit-1,arrive |
           | d,i       | def,ghi,ghi | depart,roundabout-exit-2,arrive |
           | d,l       | def,jkl,jkl | depart,roundabout-exit-3,arrive |
           | d,c       | def,abc,abc | depart,roundabout-exit-4,arrive |
           | g,i       | ghi,ghi,ghi | depart,roundabout-exit-1,arrive |
           | g,l       | ghi,jkl,jkl | depart,roundabout-exit-2,arrive |
           | g,c       | ghi,abc,abc | depart,roundabout-exit-3,arrive |
           | g,f       | ghi,edf,edf | depart,roundabout-exit-4,arrive |
           | j,l       | jkl,jkl,jkl | depart,roundabout-exit-1,arrive |
           | j,c       | jkl,abc,abc | depart,roundabout-exit-2,arrive |
           | j,f       | jkl,def,def | depart,roundabout-exit-3,arrive |
           | j,i       | jkl,ghi,ghi | depart,roundabout-exit-4,arrive |
