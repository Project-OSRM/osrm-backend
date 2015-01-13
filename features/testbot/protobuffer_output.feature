@routing @pbf_output @testbot
Feature: Outputting protobuffer format - viaroute

    Background:
        Given the profile "testbot"
        And the query options
            | output | pbf |
    
    Scenario: Testbot - Protobuffer output of single way
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |

    Scenario: Testbot - Protobuffer output of small network
        Given the node map
            | a | b |   |   |
            |   |   | c | d |
            |   |   |   | e |
            |   |   | f |   |

        And the ways
            | nodes  |
            | ab     |
            | def    |
            | bc     |
            | cd     |

        When I route I should get
            | from | to | route        |
            | a    | f  | ab,bc,cd,def |

      Scenario: Testbot - Protobuffer output of of roundabout
          Given the node map
              |   |   | v |   |   |
              |   |   | d |   |   |
              | s | a |   | c | u |
              |   |   | b |   |   |
              |   |   | t |   |   |

          And the ways
              | nodes | junction   |
              | sa    |            |
              | tb    |            |
              | uc    |            |
              | vd    |            |
              | abcda | roundabout |

          When I route I should get
              | from | to | route | turns                               |
              | s    | t  | sa,tb | head,enter_roundabout-1,destination |
              | s    | u  | sa,uc | head,enter_roundabout-2,destination |
              | s    | v  | sa,vd | head,enter_roundabout-3,destination |
              | t    | u  | tb,uc | head,enter_roundabout-1,destination |
              | t    | v  | tb,vd | head,enter_roundabout-2,destination |
              | t    | s  | tb,sa | head,enter_roundabout-3,destination |
              | u    | v  | uc,vd | head,enter_roundabout-1,destination |
              | u    | s  | uc,sa | head,enter_roundabout-2,destination |
              | u    | t  | uc,tb | head,enter_roundabout-3,destination |
              | v    | s  | vd,sa | head,enter_roundabout-1,destination |
              | v    | t  | vd,tb | head,enter_roundabout-2,destination |
              | v    | u  | vd,uc | head,enter_roundabout-3,destination |

      Scenario: Protobuffer output of bearing af 45 degree intervals
          Given the node map
              | b | a | h |
              | c | x | g |
              | d | e | f |

          And the ways
              | nodes |
              | xa    |
              | xb    |
              | xc    |
              | xd    |
              | xe    |
              | xf    |
              | xg    |
              | xh    |

          When I route I should get
              | from | to | route | compass | bearing |
              | x    | a  | xa    | N       | 0       |
              | x    | b  | xb    | NW      | 315     |
              | x    | c  | xc    | W       | 270     |
              | x    | d  | xd    | SW      | 225     |
              | x    | e  | xe    | S       | 180     |
              | x    | f  | xf    | SE      | 135     |
              | x    | g  | xg    | E       | 90      |
              | x    | h  | xh    | NE      | 45      |


      Scenario: Testbot - Protobuffer output of modes in each direction, different forward/backward speeds
          Given the node map
              |   | 0 | 1 |   |
              | a |   |   | b |

          And the ways
              | nodes | highway | oneway |
              | ab    | river   |        |

          When I route I should get
              | from | to | route | modes |
              | a    | 0  | ab    | 3     |
              | a    | b  | ab    | 3     |
              | 0    | 1  | ab    | 3     |
              | 0    | b  | ab    | 3     |
              | b    | 1  | ab    | 4     |
              | b    | a  | ab    | 4     |
              | 1    | 0  | ab    | 4     |
              | 1    | a  | ab    | 4     |

      Scenario: Testbot - Protobuffer of multiple via points
          Given the node map
              | a |   |   |   | e | f | g |   |
              |   | b | c | d |   |   |   | h |

          And the ways
              | nodes |
              | ae    |
              | ab    |
              | bcd   |
              | de    |
              | efg   |
              | gh    |
              | dh    |

          When I route I should get
              | waypoints | route                    |
              | a,c,f     | ab,bcd,bcd,de,efg        |
              | a,c,f,h   | ab,bcd,bcd,de,efg,efg,gh |


      Scenario: Testbot - Protobuffer of streetnames with UTF characters
          Given the node map
              | a | b | c | d |

          And the ways
              | nodes | name                   |
              | ab    | Scandinavian København |
              | bc    | Japanese 東京            |
              | cd    | Cyrillic Москва        |

          When I route I should get
              | from | to | route                  |
              | a    | b  | Scandinavian København |
              | b    | c  | Japanese 東京            |
              | c    | d  | Cyrillic Москва        |


      Scenario: Testbot - Protobuffer of route retrieving geometry
         Given the node locations
              | node | lat | lon |
              | a    | 1.0 | 1.5 |
              | b    | 2.0 | 2.5 |
              | c    | 3.0 | 3.5 |
              | d    | 4.0 | 4.5 |

          And the ways
              | nodes |
              | ab    |
              | bc    |
              | cd    |

          When I route I should get
              | from | to | route | geometry                             |
              | a    | c  | ab,bc | _c`\|@_upzA_c`\|@_c`\|@_c`\|@_c`\|@  |
              | b    | d  | bc,cd | _gayB_yqwC_c`\|@_c`\|@_c`\|@_c`\|@   |
