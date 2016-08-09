@routing  @guidance
Feature: Slipways and Dedicated Turn Lanes

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    Scenario: Turn Instead of Ramp
        Given the node map
            |   |   |   |   | e |   |
            | a | b |   |   | c | d |
            |   |   |   | h |   |   |
            |   |   |   |   |   |   |
            |   |   |   | 1 |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   | f |   |
            |   |   |   |   |   |   |
            |   |   |   |   | g |   |

        And the ways
            | nodes | highway    | name   |
            | abcd  | trunk      | first  |
            | bhf   | trunk_link |        |
            | ecfg  | primary    | second |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abcd     | ecfg   | c        | no_right_turn |

       When I route I should get
            | waypoints | route               | turns                           |
            | a,g       | first,second,second | depart,turn right,arrive        |
            | a,1       | first,,             | depart,turn slight right,arrive |

    Scenario: Turn Instead of Ramp
        Given the node map
            |   |   |   |   | e |   |
            | a | b |   |   | c | d |
            |   |   |   | h |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   | f |   |
            |   |   |   |   |   |   |
            |   |   |   |   |   |   |
            |   |   |   |   | g |   |

        And the ways
            | nodes | highway       | name   |
            | abcd  | motorway      | first  |
            | bhf   | motorway_link |        |
            | efg   | primary       | second |

       When I route I should get
            | waypoints | route                | turns                                             |
            | a,g       | first,,second,second | depart,off ramp slight right,turn straight,arrive |

    Scenario: Inner city expressway with on road
        Given the node map
            | a | b |   |   |   | c | g |
            |   |   |   |   | f |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   | d |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |
            |   |   |   |   |   | e |   |

        And the ways
            | nodes | highway      | name  |
            | abc   | primary      | road  |
            | cg    | primary      | road  |
            | bfd   | trunk_link   |       |
            | cde   | trunk        | trunk |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | abc      | cde    | c        | no_right_turn |

       When I route I should get
            | waypoints | route                | turns                    |
            | a,e       | road,trunk,trunk     | depart,turn right,arrive |


    Scenario: Slipway Round U-Turn
        Given the node map
            | a |   | f |
            |   |   |   |
            | b |   | e |
            |   |   |   |
            |   |   |   |
            |   | g |   |
            |   |   |   |
            | c |   | d |

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |

    Scenario: Slipway Steep U-Turn
        Given the node map
            | a |   | f |
            |   |   |   |
            | b |   | e |
            |   | g |   |
            |   |   |   |
            |   |   |   |
            | c |   | d |

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | bge   | primary_link |      | yes    |
            | def   | primary      | road | yes    |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,f       | road,road,road | depart,continue uturn,arrive |

    Scenario: Schwarzwaldstrasse Autobahn
        Given the node map
            |   |   |   |   | i |   |   |   |   |   | h |   |   |   |   | g |
            |   |   | j |   |   |   |   |   |   |   |   |   |   |   |   |   |
            | a |   |   |   |   |   |   | k |   |   |   |   |   |   |   |   |
            |   |   |   | b |   | r | c |   | d |   | e |   |   |   |   | f |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   | l |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   | m |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | n |   | q |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | o |   | p |   |   |   |   |   |   |   |

        And the nodes
            # the traffic light at `l` is not actually in the data, but necessary for the test to check everything
            # http://www.openstreetmap.org/#map=19/48.99211/8.40336
            | node | highway         |
            | r    | traffic_signals |
            | l    | traffic_signals |

        And the ways
            | nodes | highway        | name               | ref  | oneway |
            | abrcd | secondary      | Schwarzwaldstrasse | L561 | yes    |
            | hija  | secondary      | Schwarzwaldstrasse | L561 | yes    |
            | def   | secondary      | Ettlinger Strasse  |      | yes    |
            | gh    | secondary      | Ettlinger Strasse  |      | yes    |
            | blmn  | secondary_link |                    | L561 | yes    |
            | hkc   | secondary_link | Ettlinger Strasse  |      | yes    |
            | qdki  | secondary_link | Ettlinger Allee    |      | yes    |
            | cn    | secondary_link | Ettlinger Allee    |      | yes    |
            | no    | primary        | Ettlinger Allee    |      | yes    |
            | pq    | primary        | Ettlinger Allee    |      | yes    |
            | qe    | secondary_link | Ettlinger Allee    |      | yes    |

        When I route I should get
            | waypoints | route                                                     | turns                    |
            | a,o       | Schwarzwaldstrasse (L561),Ettlinger Allee,Ettlinger Allee | depart,turn right,arrive |

    Scenario: Traffic Lights everywhere
        #http://map.project-osrm.org/?z=18&center=48.995336%2C8.383813&loc=48.995467%2C8.384548&loc=48.995115%2C8.382761&hl=en&alt=0
        Given the node map
            | a |   |   | k | l |   |   | j |   |
            |   |   |   |   |   | d | b | c | i |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | e | g |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   | h |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   | f |   |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |
            | e    | traffic_signals |
            | g    | traffic_signals |

        And the ways
            | nodes  | highway        | name          | oneway |
            | aklbci | secondary      | Ebertstrasse  | yes    |
            | kdeh   | secondary_link |               | yes    |
            | jcghf  | primary        | Brauerstrasse | yes    |

        When I route I should get
            | waypoints | route                                    | turns                    |
            | a,i       | Ebertstrasse,Ebertstrasse                | depart,arrive            |
            | a,l       | Ebertstrasse,Ebertstrasse                | depart,arrive            |
            | a,f       | Ebertstrasse,Brauerstrasse,Brauerstrasse | depart,turn right,arrive |
